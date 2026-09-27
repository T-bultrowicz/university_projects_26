module Derive.ExpTree where
import Derive.Definitions
import Control.Monad.State

-- Typ obliczeń dla wyrażeń na drzewach ETree EVar typu t z kontekstem
-- wzbogaconym o typ c, oraz wynikiem będącym typu r.
type ExpTree c r = State (EZipper c) r

-- Zipper dla drzew ETree Expr.
type EZipper c = Zipper c Expr
-- Alias dla drzew ETree Expr.
type EVTree = ETree Expr

data ETree a = Tip a | Bin (ETree a) (ETree a)
instance Show a => Show (ETree a) where
  showsPrec _ (Tip v) = shows v
  showsPrec 0 (Bin l r) = showsPrec 0 l . showChar ' ' . showsPrec 1 r
  showsPrec _ (Bin l r) = showChar '(' . showsPrec 0 (Bin l r)  . showChar ')'

-- Kontekst do przemieszczania się po drzewach typu t.
data Cxt t = Top | L (ETree t) (Cxt t) | R (ETree t) (Cxt t)

-- Klasyczny zipper rozbudowany o możliwość przechowywania dodatkowego stanu 
-- typu c, który może być wykorzystywany w obliczeniach.
data Zipper c t = Zipper 
  { zipCache :: c
  , zipTree  :: (ETree t)
  , zipCxt   :: (Cxt t)
  }

-- Funkcja pomocnicza do modyfikowania stanu cache w zipperze.
modifyCache :: (c -> c) -> Zipper c t -> Zipper c t
modifyCache f z = z { zipCache = f (zipCache z) }

-- Pomocniczy typ drzewa umożliwiający wyświetlanie zippera jako drzewa 
-- z zaznaczoną aktualną pozycją.
data MarkedTree a 
  = MTip a 
  | NTip a 
  | MBin (MarkedTree a) (MarkedTree a)
  | NBin (MarkedTree a) (MarkedTree a)

-- Pokzywanie drzewa z zaznaczeniem aktualnej pozycji (MTip/MBin).
instance Show a => Show (MarkedTree a) where
  showsPrec _ (NTip v) = shows v
  showsPrec 0 (NBin l r) = showsPrec 0 l . showChar ' ' . showsPrec 1 r
  showsPrec _ (NBin l r) = showChar '(' . showsPrec 0 (NBin l r) . showChar ')'
  showsPrec _ (MTip v) = marked (showsPrec 0 (NTip v))
    where
      marked x = showChar '{' . x . showChar '}'
  showsPrec _ (MBin l r) = marked (showsPrec 0 (NBin l r))
    where
      marked x = showChar '{' . x . showChar '}'

-- Wypisanie zippera by umożliwić zapis procesu derywacji. Polega na konwersji
-- zwykłegi drzewa ETree do MarkedTree z zaznaczoną aktualną pozycją.
toMarkedFocus :: ETree a -> MarkedTree a
toMarkedFocus (Tip v) = MTip v
toMarkedFocus (Bin l r) = MBin (toMarkedNormal l) (toMarkedNormal r)

toMarkedNormal :: ETree a -> MarkedTree a
toMarkedNormal (Tip v) = NTip v
toMarkedNormal (Bin l r) = NBin (toMarkedNormal l) (toMarkedNormal r)

instance Show t => Show (Zipper c t) where
  showsPrec d (Zipper _ t cx) = showsPrec d (genMarkedTree (toMarkedFocus t) cx)
    where
      genMarkedTree curr Top       = curr
      genMarkedTree curr (L r cxt) = genMarkedTree (NBin curr (toMarkedNormal r)) cxt
      genMarkedTree curr (R l cxt) = genMarkedTree (NBin (toMarkedNormal l) curr) cxt
      
-- Funkcje konwersji między typami.
fromExp :: Expr -> EVTree
fromExp (e1 :$ e2) = Bin (fromExp e1) (fromExp e2)
fromExp t = Tip t

newState :: Expr -> c -> EZipper c
newState ex cache = Zipper cache (fromExp ex) Top

toExp :: EVTree -> Expr
toExp (Bin l r) = toExp l :$ toExp r
toExp (Tip t) = t

-- Własny foldr, ponieważ definiowanie instancji Foldable dla ETree w sposób
-- klasyczny nie spełnia moich potrzeb, a w ten sposób nie byłoby możliwe dla
-- każdego typu ETree.
foldTree :: (Expr -> b -> b) -> b -> EVTree -> b
foldTree f z (Tip x) = f x z
foldTree f z (Bin l r) = foldTree f (f (toExp r) z) l

toList :: EVTree -> [Expr]
toList = foldTree (:) []

-- Parę bazowych funkcji do poruszania się po drzewie i modyfikacji.
-- Korzystają z typu DeriveResult, aby umożliwić łatwiejsze łączenie operacji na
-- podstawie ich wyników, za pomocą operatorów >>? i !>>?.
type DeriveResult = Maybe ()
success :: DeriveResult
success = Just ()
failure :: DeriveResult
failure = Nothing

left :: ExpTree c DeriveResult
left = do
  z <- get
  case zipTree z of
    Bin l r -> do
      put  $ Zipper (zipCache z) l (L r (zipCxt z))
      pure $ Just ()
    _ -> pure Nothing

bottomleft :: ExpTree c DeriveResult
bottomleft = left >>? bottomleft

right :: ExpTree c DeriveResult
right = do
  z <- get
  case zipTree z of
    Bin l r -> do
      put  $ Zipper (zipCache z) r (R l (zipCxt z))
      pure $ Just ()
    _ -> pure Nothing

up :: ExpTree c DeriveResult
up = do
  z <- get
  case zipCxt z of
    Top -> pure Nothing
    L r cxt -> do
      put  $ Zipper (zipCache z) (Bin (zipTree z) r) cxt
      pure $ Just ()
    R l cxt -> do
      put  $ Zipper (zipCache z) (Bin l (zipTree z)) cxt
      pure $ Just ()

climbTop :: ExpTree c DeriveResult
climbTop = (up >>? climbTop) !>>? pure success

insertHere :: Expr -> ExpTree c ()
insertHere e =  (modify $ \z -> z { zipTree = fromExp e }) >> pure ()

-- Operatory uproszczające sekwencjonowanie obliczeń ze stanem z wynikiem Maybe.
-- Kontynuuj do pierwszego failure
(>>?) :: ExpTree c (Maybe a) -> ExpTree c (Maybe a) -> ExpTree c (Maybe a)
first >>? second = do
  res <- first
  case res of 
    Nothing -> pure Nothing
    Just _  -> second

-- Kontynuuj do pierwszego success
(!>>?) :: ExpTree c (Maybe a) -> ExpTree c (Maybe a) -> ExpTree c (Maybe a)
first !>>? second = do
  res <- first
  case res of
    Just a -> pure $ Just a
    Nothing -> second

