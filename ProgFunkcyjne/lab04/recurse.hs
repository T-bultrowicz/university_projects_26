data Nat = Zero | S Nat deriving Show

instance Eq Nat where
    Zero == Zero = True
    S m == S n = m == n
    _ == _ = False

instance Ord Nat where
    compare Zero  Zero  = EQ
    compare Zero  (S _) = LT
    compare (S _) Zero  = GT
    compare (S m) (S n) = compare m n

instance Enum Nat where
    fromEnum Zero = 0
    fromEnum (S m) = 1 + (fromEnum m)
    toEnum x
        | x < 0 = error "Crash"
        | x == 0 = Zero
        | otherwise = S (toEnum (x - 1))

add :: Nat -> Nat -> Nat
add n Zero = n
add n (S m) = S (add n m)

sub :: Nat -> Nat -> Nat
sub n Zero = n
sub (S n) (S m) = sub n m

mul :: Nat -> Nat -> Nat
mul n Zero = Zero
mul n (S m) = add (mul n m) n

pot :: Nat -> Nat -> Nat
pot n Zero = (S Zero)
pot n (S m) = mul (pot n m) n

dziel :: Nat -> Nat -> (Nat, Nat)
dziel n m = case n < m of
    True  -> (Zero, n)
    False -> (S c, d)
    where
        (c, d) = dziel (sub n m) m

foldn :: (a -> a) -> a -> Nat -> a
foldn h c Zero  = c
foldn h c (S n) = h(foldn h c n)

fact = snd . foldn f (Zero, S Zero) where
  f :: (Nat, Nat) -> (Nat, Nat)
  f (m, n) = (S m, mul (S m) n)

fib = snd . foldn g (Zero, S Zero) where
  g (m, n) = (n, add m n)

npred :: Nat -> Nat
npred = fst . foldn g (Zero, False) where
    g (a, False) = (a, True)
    g (a, _) = (S a, True)
    

three = S (S (S Zero))
five = add three (S (S Zero))
nine = add (S five) three

myHead :: [a] -> a
myHead [] = error "trying to access head of an empty list"
myHead (x:xs) = x

myTail :: [a] -> [a]
myTail [] = []
myTail (x:xs) = xs

myLast :: [a] -> a
myLast [] = error "accesing last elem of an empty list"
myLast [x] = x
myLast (x:xs) = myLast xs

myInit :: [a] -> [a]
myInit [] = error "accessing init of an empty list"
myInit [x] = []
myInit (x:xs) = x:(myInit xs)

myTake :: Int -> [a] -> [a]
myTake 0 _ = []
myTake _ [] = []
myTake b (x:xs) = x:(myTake (b - 1) xs)

myDrop :: Int -> [a] -> [a]
myDrop 0 t = t
myDrop _ [] = []
myDrop b (x:xs) = (myDrop (b - 1) xs)

myReplicate :: Int -> a -> [a]
myReplicate 0 a = []
myReplicate x a = a:(myReplicate (x-1) a)

myReverse :: [a] -> [a]
myReverse [] = []
myReverse (x:xs) = myReverse xs ++ [x]

myLength :: [a] -> Int
myLength [] = 0
myLength (x:xs) = myLength xs + 1

myNull :: [a] -> Bool
myNull [] = True
myNull _ = False

myFair :: [a] -> [a] -> [a]
myFair a [] = a
myFair [] a = a
myFair (x:xs) (y:ys) = x : (y : myFair xs ys)

myMerge :: Ord a => [a] -> [a] -> [a]
myMerge a [] = a
myMerge [] a = a
myMerge (x:xs) (y:ys)
    | x < y     = x:(myMerge xs (y:ys))
    | otherwise = y:(myMerge (x:xs) ys)

myMergeSort :: Ord a => [a] -> [a]
