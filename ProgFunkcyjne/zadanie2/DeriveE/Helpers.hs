module DeriveE.Helpers where

import DeriveE.Definitions
import DeriveE.FromHs
import qualified Data.Map as Map

-- Dummy used in insertWithLookup.
dummyFoo :: k -> () -> () -> ()
dummyFoo _ () () = ()

-- Creates unique param.
createParam :: Int -> Pat
createParam x = "Param_" ++ (show x)

-- Our main will be transformed to this constant.
mainName :: String
mainName = "MainFunction"

-- Creates unique function.
renameFunc :: Def -> Int -> Pat
renameFunc def intID = case extractName def of
  "main"    -> mainName
  _         -> "Fun_" ++ (show intID)

-- Lookup useful in name substitution.
myLookup :: Ord a => Map.Map a a -> a -> a
myLookup mp el = case Map.lookup el mp of
  Just a  -> a
  Nothing -> el

-- Check uniqueness of args. Returns map of args (useful to find main).
checkUnique :: Ord a => [a] -> (Map.Map a (), Maybe Err)
checkUnique [] = (Map.empty, Nothing)
checkUnique (x:xs) = case checkUnique xs of 
  (_, Just err) -> (Map.empty, Just err)
  (mapp, Nothing) -> case Map.insertLookupWithKey dummyFoo x () mapp of
    (Just _, _)         -> (Map.empty, Just  "Parameter name repeated in function!\n")
    (Nothing, new_mapp) -> (new_mapp,  Nothing)


-- Check for two bad rules: main with args, repeated args of function.
isWrongDef :: Def -> Either Err Def
isWrongDef (Def "main" [] expr) = Right (Def "main" [] expr)
isWrongDef (Def "main" _  _)    = Left "Main with args detected!\n"
isWrongDef (Def name t expr)    = case checkUnique t of
  (_, Just err) -> Left err
  (_, Nothing)  -> Right (Def name t expr)


-- Check correctness of prog given by fromHsModuleUnchecked function.
checkProg :: Either Err Prog -> Either Err Prog
checkProg (Left err) = Left err
checkProg (Right p) = case sequenceEither (map isWrongDef (progDefs p)) of
  Left err   -> Left err
  Right prog -> case checkUnique (map extractName prog) of
    (_, Just _)     -> Left "Multiple function definitions provided!\n"
    (mapp, Nothing) -> case Map.lookup "main" mapp of
      Nothing -> Left "Main was not detected in file!\n"
      Just _  -> Right (Prog prog)

changeNameExpr :: RenameMap -> RenameMap -> Expr -> Expr
changeNameExpr map1 map2 (Var n) = case Map.lookup n map1 of
  Just new_n -> (Var new_n)
  Nothing    -> (Var (myLookup map2 n))
changeNameExpr m1 m2 (a :$ b) = (changeNameExpr m1 m2 a) :$ (changeNameExpr m1 m2 b)

changeNameDef :: RenameMap -> Def -> Def
changeNameDef changeMap (Def n pats expr) = (Def (changeMap Map.! n) 
  (take (length pats) constPatList) (changeNameExpr changeMap mapPat expr))
  where
    constPatList = map createParam [0..]
    mapPat       = Map.fromList (zip pats constPatList)

buildChangeMap :: [Def] -> (RenameMap, Int)
buildChangeMap []     = (Map.empty, 0)
buildChangeMap (x:xs) = (Map.insert (extractName x) (renameFunc x numId) mapp, numId + 1)
  where
    (mapp, numId) = buildChangeMap xs

-- creates map where values are the new keys and vice-versa
revPair :: (k, v) -> (v, k)
revPair (a, b) = (b, a)

reverseMap :: RenameMap -> RenameMap
reverseMap m = Map.fromList (map revPair (Map.assocs m))

-- Return uniquely renamed prog, along with map allowing to switch names back.
renameUnique :: Prog -> (RenameMap, Prog)
renameUnique prog = (reverseMap changeMap, newProg)
  where
    pDefs          = progDefs prog
    (changeMap, _) = buildChangeMap pDefs
    newProg        = Prog (map (changeNameDef changeMap) pDefs)

-- Flip names inside expression.
renameBack :: RenameMap -> Expr -> Expr
renameBack rm (Var n)  = Var (myLookup rm n)
renameBack rm (a :$ b) = (renameBack rm a :$ renameBack rm b)

-- Returns main expr.
findMainExpr :: DefMap -> Expr
findMainExpr mp = case Map.lookup mainName mp of
  Just def -> extractExpr def
  Nothing  -> error "Unexpected error - didn't find main, although should have!\n"

-- Pretty prits [Expr] representing derivation path.
prettyExprs :: [Expr] -> (String -> String)
prettyExprs []     = id
prettyExprs (x:xs) = shows x . showChar '\n' . prettyExprs xs

