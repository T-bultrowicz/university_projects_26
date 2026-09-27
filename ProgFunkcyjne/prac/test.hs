import Control.Monad.Reader
import Control.Monad.State
import Control.Applicative

data ETree a = Tip a | Bin (ETree a) (ETree a) deriving Show

etree :: ETree Int
etree = Bin (Bin (Tip 1) (Tip 2)) (Bin (Tip 3) (Tip 4))

instance Foldable ETree where
  foldr f z (Tip a) = (f a z)
  foldr f z (Bin l r) = foldr f (foldr f z l) r

  foldMap f (Tip a) = f a
  foldMap f (Bin l r) = foldMap f l <> foldMap f r

instance Functor ETree where
  fmap f (Tip a) = Tip (f a)
  fmap f (Bin l r) = Bin (fmap f l) (fmap f r)

instance Applicative ETree where
  pure a = Tip a
  (<*>) (Tip f) (Tip a) = Tip (f a)
  (<*>) (Tip f) (Bin a b) = fmap f (Bin a b)
  (<*>) (Bin l r) (Tip a) = l <*> (Tip a)
  (<*>) (Bin l r) (Bin a b) = Bin (l <*> a) (r <*> b)

instance Traversable ETree where
  traverse f (Tip a) = Tip <$> f a
  traverse f (Bin l r) = Bin <$> traverse f l <*> traverse f r

instance Monad ETree where
  return = pure
  (Tip a) >>= f = f a
  (Bin l r) >>= f = Bin (l >>= f) (r >>= f)

newtype Arr a b = Arr { apply ::  a -> b }

instance Functor (Arr a) where
  fmap f arra = Arr (f . (apply arra))

instance Applicative (Arr a) where
  pure x = Arr (const x)
  (<*>) (Arr fabc) (Arr fab) = Arr (\a -> fabc a (fab a))

instance Monad (Arr a) where
  return = pure
  Arr f >>= afb = Arr (\a -> apply (afb (f a)) a)

instance MonadReader r (Arr r) where
  local tr st = Arr (apply st . tr)
  ask = Arr id

instance MonadState r (Arr r) where
  get = Arr id
  put r = Arr (const r) >> pure ()

-- instance Alternative (Arr r) where
--   empty = Arr (const ())
--   a1 <|> a2 = 

newtype Tab a = Tab { tab :: [a] }

instance Foldable Tab where
  foldr f z (Tab []) = z
  foldr f z (Tab (a:as)) = foldr f (f a z) as

  foldMap f (Tab []) = mempty f
  foldMap f (Tab (a:as)) = f a <> foldMap f as

instance Functor Tab where
    fmap f (Tab a) = Tab (map f a)

instance Applicative Tab where
  pure x = (Tab [x])
  (<*>) (Tab []) (Tab []) = Tab []
  (<*>) (Tab (f:fs)) (Tab (a:as)) = Tab (f a: tab (Tab fs <*> Tab as))

instance Alternative Tab where
  empty = Tab []
  (<|>) (Tab x) (Tab y) = Tab (x ++ y)
  
binar :: Int -> [Int]
binar 0 = []
binar x = (mod x 2):(binar (div x 2))

example :: Arr Int [Int]
example = Arr binar