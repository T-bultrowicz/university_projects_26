-- Tymoteusz Bultrowicz tb469198 - 03.03.2026
-- 1 zadanie zaliczeniowe - Programowanie Funkcyjne

-- Expr data type
data Expr = S | K | I | B 
          | Expr :$ Expr 
          | X | Z | V Int  
          deriving (Show, Read)
infixl 9 :$

--------------------------------- EXPR PRINTING ---------------------------------

------------- prettyExpr -------------

-- Prints out single expression in readable way.
prettyExpr :: Expr -> (String -> String)

-- complex expression on right - parentheses needed
prettyExpr (a :$ (b :$ c)) = prettyExpr a . showString " (" . prettyExpr (b :$ c) . showChar ')'
-- print recursion
prettyExpr (a :$ b) = prettyExpr a . showChar ' ' . prettyExpr b

-- end of recursion - change way to print vars
prettyExpr X = showChar 'x'
prettyExpr Z = showChar 'z'
prettyExpr (V n) = showChar 'v' . shows n
prettyExpr a = shows a

-------------- showExprs --------------

showExprs :: [Expr] -> (String -> String)
showExprs [a] = prettyExpr a
showExprs (a:b) = prettyExpr a . showChar '\n' . showExprs b

--------------------------------- STEP FUNCTIONS ---------------------------------

-------------- normalStep --------------

-- Derives from outermost, returns tuple containing:
-- 1. Result of step    2. Change-made flag (False = stop deriving).
normalStep :: Expr -> (Expr, Bool)

-- rules for applying S, K, I, B formulas
normalStep (S :$ a :$ b :$ c) = (((a :$ c) :$ (b :$ c)), True)
normalStep (K :$ a :$ b) = (a, True)
normalStep (I :$ a) = (a, True)
normalStep (B :$ a :$ b :$ c) = ((a :$ (b :$ c)), True)

-- derive recursion - try first left side, then right side
normalStep (a :$ b) = case normalStep a of
    (res, True) -> (res :$ b, True)
    (_, False) -> (a :$ c, d)
    where
        (c, d) = normalStep b

-- stop recursion rule
normalStep a = (a, False)

---------------- i_kStep -----------------

-- helper for cleanStep, tries to apply I / K operation
i_kStep :: Expr -> (Expr, Bool)

-- applying I / K
i_kStep (K :$ a :$ b) = (a, True)
i_kStep (I :$ a) = (a, True)

-- recursion - first try left, if fails then right
i_kStep (a :$ b) =  case i_kStep a of
    (res, True) -> (res :$ b, True)
    (_,  False) -> (a :$ c, d) 
    where
        (c, d) = i_kStep b

-- stop recursion rule
i_kStep a = (a, False)

--------------- cleanStep ----------------

-- Clean step tries to prioritise I & K, returns in same way as normalStep.
cleanStep :: Expr -> (Expr, Bool)
cleanStep a = case i_kStep a of
    (res, True) -> (res, True)
    (_,  False) -> normalStep a

--------------------------------- PATH EVAL ---------------------------------

-- calculates path of deriving expression using given step function.
evalPath :: (Expr -> (Expr, Bool)) -> (Expr, Bool) -> [Expr]
-- false = stop recursion
evalPath pathFn (e, False) = []
-- true = keep on recursing
evalPath pathFn (e, True) = (e : evalPath pathFn (pathFn e))

--------------------------------- PRINT PATH ---------------------------------

-- printPath with finite output of max 30 steps & using normal derivation process
printPath :: Expr -> IO()
printPath  e = putStrLn (showExprs (take 30 (evalPath normalStep (e, True))) "")

-- printPath' - tries to firstly make expression shorter
printPath' :: Expr -> IO()
printPath' e = putStrLn (showExprs (take 30 (evalPath  cleanStep (e, True))) "")

--------------------------------- TEST UNITS ---------------------------------
test1 = S :$ K :$ K :$ X
twoB = S :$ B :$ I
threeB = S :$ B :$ (S :$B :$ I)
test3 = threeB :$ X :$ Z
omega = ((S :$ I) :$ I) :$ ((S :$ I) :$ I)
kio = K :$ I :$ omega
add = (B :$ S) :$ (B :$ B)