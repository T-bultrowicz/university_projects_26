elimMaybe :: c -> (a -> c) -> Maybe a -> c
elimMaybe _ f (Just a) = f a
elimMaybe c _ Nothing = c

fromMaybe :: a -> Maybe a -> a
fromMaybe _ (Just opt) = opt
fromMaybe o _ = o

mapMaybe :: (a -> b) -> Maybe a -> Maybe b
mapMaybe f (Just opt) = Just (f opt)
mapMaybe _ _ = Nothing

maybeHead :: [a] -> Maybe a
maybeHead (head:tail) = Just head
maybeHead _ = Nothing

elimEither :: (a  -> c) -> (b -> c) -> Either a b -> c
elimEither f _ (Left arg) = f arg
elimEither _ g (Right arg) = g arg

mapEither :: (a1 -> a2) -> (b1 -> b2) -> Either a1 b1 -> Either a2 b2
mapEither f _ (Left arg) = Left (f arg)
mapEither _ g (Right arg) = Right (g arg)

mapRight ::  (b1 -> b2) -> Either a b1 -> Either a b2
mapRight f (Right b) = Right (f b)
mapRight _ (Left a) = Left a

fromEither :: Either a a -> a
fromEither (Left a) = a
fromEither (Right a) = a

both :: (a -> b) -> (a, a) -> (b, b)
both f (x, y) = (f x, f y)

all3 :: (a -> b) -> (a, a, a) -> (b, b, b)
all3 f (x, y, z) = (f x, f y, f z)

first :: (a -> c) -> (a, b) -> (c, b)
first f (x, y) = (f x, y)

second :: (b -> d) -> (a, b) -> (a, d)
second f (x, y) = (x, f y)

dividors x = [y | y <- [2..x - 1], mod x y == 0]
isPrime x = (dividors x == [])

triads x = [(a, b, c) | a <- [1..x], b <- [a..x], c <- [b.. x], a*a + b*b == c*c]

data Tree a = Empty | Node a (Tree a) (Tree a) deriving (Eq, Ord, Show)

recTree :: Int -> Int -> Int -> Tree Int
recTree 0 _ _ = Empty
recTree n m h = Node (m + h) (recTree (n - 1) (div m 2) h) (recTree (n - 1) (div m 2) (h + m))

fullTree :: Int -> Tree Int
fullTree 0 = Empty
fullTree n = recTree n (2 ^ (n - 1)) 0

toList :: Tree a -> [a]
toList Empty = []
toList (Node x left right) = (toList left) ++ [x] ++ (toList right)

fullTreeFrom :: Int -> Int -> Tree Int
fullTreeFrom n x = recTree n (2 ^ (n - 1)) (x - 1)

rightistFromTo :: Int -> Int -> Tree Int
rightistFromTo a b | a == b    = (Node a Empty Empty)
                   | otherwise = (Node a Empty (rightistFromTo (a + 1) b))

leftistFromTo :: Int -> Int -> Tree Int
leftistFromTo a b | a == b    = (Node a Empty Empty)
                  | otherwise = (Node a (leftistFromTo (a + 1) b) Empty)
