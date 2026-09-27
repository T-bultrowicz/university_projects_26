import Data.List


data Tree paramT = E | N paramT (Tree paramT) (Tree paramT)

instance Show a => Show (Tree a) where
    showsPrec _ E = id
    showsPrec _ (N a b c) = shows a . showString " (" . shows b . showString ") (" . shows c . showString ")"

instance Eq a => Eq (Tree a) where
    E == E = True
    (N a1 b1 c1) == (N a2 b2 c2) = (a1 == a2) && (b1 == b2) && (c1 == c2)
    _  == _  = False

instance Read a => Read (Tree a) where
    readsPrec _ (' ':xs) = readsPrec 1 xs
    readsPrec _ ('(':')':xs) = [(E, xs)]
    readsPrec _ ('(':xs) =
        [(N val l r, rest4) |
            (val, rest1) <- reads xs,
            (l, rest2) <- readsPrec 1 rest1,
            (r, rest3) <- readsPrec 1 rest2,
            (")", rest4) <- lex rest3]
    readsPrec _ _ = []

inzert :: (Ord a) => a -> Tree a -> Tree a
inzert x E = N x E E
inzert x (N val l r)
    | x < val   = (N val (inzert x l) r)
    | x == val  = (N val l r)
    | otherwise = (N val l (inzert x r))

contains :: (Ord a) => a -> Tree a -> Bool
contains x E = False
contains x (N val l r)
    | x < val   = contains x l
    | x == val  = True
    | otherwise = contains x r

fromList :: (Ord a) => [a] -> Tree a
fromList [] = E
fromList (a:b) = (N a (fromList c) (fromList d))
    where
        (c, d) = partition (<a) b

eraseBiggest :: (Ord a) => Tree a -> (a, Tree a)
eraseBiggest (N val l r) = case r of
    E -> (val, l)
    (N _ _ _) -> (c, N val l d)
    where
        (c, d) = eraseBiggest r

delite :: (Ord a) => a -> Tree a -> Tree a
delite x E = E
delite x (N val l r)
    | x < val   = (N val (delite x l) r)
    | x > val   = (N val l (delite x r))
    | otherwise = case l of
        E -> r
        (N val' l' r') -> (N c d r)
        where (c, d) = eraseBiggest l


demo = N 4 (N 2 (N 1 E E) E) (N 7 (N 6 E E) (N 11 E E))



data Exp
    = EInt Int             -- stała całkowita
    | EAdd Exp Exp         -- e1 + e2
    | EMul Exp Exp         -- e1 * e2
    | EVar String          -- zmienna
    | ELet String Exp Exp  -- let var = e1 in e2

instance Eq Exp where
    EInt x == EInt y = x == y
    EVar x == EVar y = x == y
    EAdd x y == EAdd x' y' = x == x' && y == y'
    EMul x y == EMul x' y' = x == x' && y == y'
    _ == _  = False

instance Show Exp where
    showsPrec _ (EInt n)
        | n < 0 = showChar '(' . shows n . showChar ')'
        | otherwise = shows n

    showsPrec _ (EVar n) = showString n

    showsPrec p (EMul a b)
        | a == (EInt (-1)) = showString "(-" . showsPrec 4 b . showChar ')'
        | p == 4    = showsPrec 4 a . showString " * " . showsPrec 7 b
        | p < 7     = showChar '(' . showsPrec 7 (EMul a b) . showChar ')'
        | otherwise = showsPrec 4 a . showString " * " . showsPrec 7 b

    showsPrec p (EAdd a b)
        | p == 7    = showChar '(' . showsPrec 4 a . showString " + " . showsPrec 5 b . showChar ')'
        | otherwise = showsPrec 4 a . showString " + " . showsPrec 5 b

instance Num Exp where
    (+) a b = EAdd a b
    (*) a b = EMul a b
    (-) a b = EAdd a (EMul (EInt (-1)) b)
    negate a = EMul (EInt (-1)) a
    fromInteger n = EInt (fromInteger n)