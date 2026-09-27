module Derive.Derive (
  module Derive.Definitions,
  fromHsModule,
  derive
) where
import Derive.Definitions
import Derive.ExpTree
import Derive.FromHs
import Control.Monad
import Control.Monad.State

import qualified Data.Map as Map
type DefMap = Map.Map Name Def
type SubstMap = Map.Map Name Expr

data DeriveState = DeriveState {
  funs  :: DefMap,
  fuel  :: Int,
  hist  :: [String]
}

fuelMax :: Int
fuelMax = 30
bgnExpr :: Expr
bgnExpr = Var "main"
createCache :: Prog -> DeriveState
createCache p = DeriveState dictMap fuelMax []
  where
    defs = progDefs p
    dictMap = Map.fromList $ (zip (map getName defs) defs)

-- Funkcje odpowiedzialne za aplikację podstawień, tworzenie podmian zmiennych.
buildSubst :: [Expr] -> Match -> SubstMap
buildSubst exprs m = gatherSubst (zip (matchPats m) exprs)
  where
    gatherSubst = foldr (Map.union . singleGather) Map.empty
    singleGather (PApp _ ps, e) = gatherSubst (zip ps (tail $ toList $ fromExp e))
    singleGather (PVar n, e) = Map.singleton n e

makeSubst :: [Expr] -> Match -> Expr
makeSubst exprs m = myConcat (myMap (buildSubst exprs m) (matchRhs m)) rest
  where
    myMap subMap (Var n) = Map.findWithDefault (Var n) n subMap
    myMap _ (Con n) = Con n
    myMap subMap (e1 :$ e2) = myMap subMap e1 :$ myMap subMap e2
    rest = drop (length (matchPats m)) exprs
    myConcat e ts = foldl (:$) e ts

-- Funkcja dodająca krok do historii.
addHistory :: String -> ExpTree DeriveState ()
addHistory step = modify $ modifyCache $ \c -> c { hist = step : hist c }

-- Funkcje odpowiedzialne za dopasowanie wzorca do wyrażenia.
data MatchResult = Compatible | Incompatible | Unknown deriving Show
tryMatchList :: [Pat] -> [Expr] -> MatchResult
tryMatchList ps args = 
  if length ps > length args 
    then Incompatible 
    else evalList (zip ps args)
  where
    evalList :: [(Pat, Expr)] -> MatchResult
    evalList [] = Compatible
    evalList ((p, e): rest) = do
      case evalList rest of
        Incompatible -> Incompatible
        Compatible -> tryMatch p e
        Unknown -> case tryMatch p e of
          Incompatible -> Incompatible
          _            -> Unknown 

tryMatch :: Pat -> Expr -> MatchResult
tryMatch (PVar _) _ = Compatible
tryMatch (PApp n ps) expr = case exprList of
    [] -> error "tryMatch empty expr shouldn't happen"
    (Con m : args) -> do
      if n == m
        then tryMatchList ps args
        else Incompatible
    _  -> Unknown
  where
    exprList = toList (fromExp expr)

-- Funkcja rekurencyjna próbująca dopasować wzorzec do wyrażenia, 
-- wykonująca redukcje pośredniczące.
recCheckCompatibility :: Pat -> Bool -> ExpTree DeriveState DeriveResult
recCheckCompatibility pat shouldPrint = do
  zt <- gets (toExp . zipTree)
  case tryMatch pat zt of
    Compatible   -> pure success
    Incompatible -> pure failure
    -- Wypisujemy do historii konieczność wykonania redukcji pośredniczącej.
    Unknown      -> do
      -- Wywoływanie printowania tylko przy 1. wejściu do funkcji, pozwala uniknąć
      -- powieleń poprzez wypisanie wyrażenia wewnątrz tryReduce i tutaj.
      if shouldPrint
        then do
          st <- get
          addHistory (show st)
      else pure ()
      -- Wykonanie redukcji pośredniczącej.
      rStep >>? recCheckCompatibility pat False

-- Funckja owija recCheckCompatibility i pilnuje by zachować niezmiennik
-- pozostania zippera na startowej pozycji.
checkCompatibility :: [Pat] -> ExpTree DeriveState DeriveResult
checkCompatibility [] = pure success
checkCompatibility (p:ps) = do
  void up
  void right
  res <- recCheckCompatibility p True
  void up
  if res == success
    then checkCompatibility ps
    else fallBackToStateBefore ps

fallBackToStateBefore :: [Pat] -> ExpTree DeriveState DeriveResult
fallBackToStateBefore [] = pure failure
fallBackToStateBefore (_:ps) = up >> fallBackToStateBefore ps

upNSteps :: Int -> ExpTree c ()
upNSteps 0 = pure ()
upNSteps n = up >> upNSteps (n - 1)

-- Owinięcie wszystkich powyższych funkcji, które zwraca sprawdzany wzorzec,
-- jeśli można go zaaplikować do aktualnej pozycji, ew. zmienionej 
-- redukcjami pośredniczącymi.
isApplicable :: Match -> ExpTree DeriveState (Maybe Match)
isApplicable (Match n [] rhs) = pure $ Just $ (Match n [] rhs)
isApplicable m = do
  st <- gets zipTree
  let tr = tail $ toList st
  let pats = matchPats m
  if length tr < length pats then pure Nothing else do
    void bottomleft
    res <- checkCompatibility pats
    upNSteps (length tr - (length pats))
    case res == success of
      True -> pure $ Just m
      False -> pure Nothing

-- Funkcje pomocnicze dla TryReduce, pozwalająca na zaczęcie procesu wyprowadzania.
searchFirstVar :: ExpTree DeriveState Name
searchFirstVar = do
  void bottomleft
  tree <- gets zipTree
  case tree of
    Tip (Var n) -> pure n
    Tip (Con n) -> pure n
    _           -> pure ""

-- Funckja zwracająca pierwszą zmienną w drzewie wyrażeń.
firstVar :: ExpTree DeriveState Name
firstVar = do
  save <- get
  nm <- searchFirstVar
  put save
  pure nm

firstApplicable :: [Match] -> ExpTree DeriveState (Maybe Match)
firstApplicable [] = pure Nothing
firstApplicable (m:ms) = isApplicable m !>>? firstApplicable ms

-- Właściwe wykonanie pojedynczej redukcji, 
-- przez znalezienie pierwszego poprawnego matcha.
tryReduce :: ExpTree DeriveState DeriveResult
tryReduce = do
  v <- firstVar
  cache <- gets zipCache
  case Map.lookup v (funs cache) of
    Just (Def mtchs) -> firstApplicable mtchs >>= \r -> case r of
      Just m -> do
        expTable' <- gets takeTreeTable
        insertHere (makeSubst expTable' m) -- Podstawiamy pasujące wyrażenie.
        st <- get -- Zapisujemy ten krok w historii.
        addHistory (show st)
        pure success
      Nothing -> pure failure
    _ -> pure failure
  where
    takeTreeTable = tail . toList . zipTree
      
-- Przeszukiwanie drzewa wyrażeń, próbując zastosować reguły redukcji.
searchLeft :: ExpTree DeriveState DeriveResult
searchLeft = do
  resL <- left
  if resL == success then do
    res <- treeSearch' -- treeSearch to backup
    void up
    pure res
  else pure failure

searchRight :: ExpTree DeriveState DeriveResult
searchRight = do
  resR <- right
  if resR == success then do
    res <- treeSearch
    void up
    pure res
  else pure failure

treeSearch' :: ExpTree DeriveState DeriveResult
treeSearch' = searchLeft !>>? searchRight

treeSearch :: ExpTree DeriveState DeriveResult
treeSearch = tryReduce !>>? searchLeft !>>? searchRight

-- Próba pojedynczego zaaplikowania reguł redukcji (step), przez próby
-- dopasowania wzorca może wykonywać redukcje pośredniczące. 
rStep :: ExpTree DeriveState DeriveResult
rStep = do
  f <- gets (fuel . zipCache)
  if f <= 0 then pure failure else
    treeSearch >>? reduceFuel
  where
    suc = pure . (const success)
    reduceFuel = (modify $ modifyCache $ \c -> c { fuel = fuel c - 1 }) >>= suc

-- Rekurencyjnie aplikuje rStep, dopóki redukcje są możliwe (Success).
-- Gdy redukcje się skończą (Failure), wypisuje historię kroków.
rPath :: ExpTree DeriveState (String -> String)
rPath = climbTop >> rStep >>= \t -> case t == success of
    True -> rPath
    False -> do
      steps <- gets (hist . zipCache)
      pure $ showString (preparePrint steps)
  where
    -- Ponieważ by optymalizować doklejenie w O(1) trzymaliśmy listę,
    -- w odwrotnej kolejności, teraz trzeba ją odwrócić. Ponadto doklejamy
    -- znak nowej linii po każdym kroku, żeby wynik był czytelny.
    preparePrint = concat . reverse . map (++ "\n")

-- Główna funkcja, która przyjmuje program i zwraca string reprezentujący
-- historię kroków jego wyprowadzania.
derive :: Prog -> (String -> String)
derive p = showString "{main}\n" . evalState rPath (newState bgnExpr (createCache p))