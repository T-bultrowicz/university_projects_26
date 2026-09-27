module DeriveE.Reduce where
import DeriveE.Definitions
import qualified Data.Map as Map

type StepFunction = DefMap -> Expr -> Maybe Expr

-- Flatten expr from left to right.
flattenExpr :: Expr -> [Expr]
flattenExpr (Var n) = [Var n]
flattenExpr (a :$ b) = flattenExpr a ++ [b]

-- Reverses effects of flatten. Maybe -> it can't work on empty list.
unflattenExpr :: [Expr] -> Maybe Expr
unflattenExpr [] = Nothing
unflattenExpr (x:xs) = Just (foldl myApp x xs)

myApp :: Expr -> Expr -> Expr
myApp a b = (a :$ b)

-- Returns expression with args substituted according to map provided.
subst :: ExprssMap -> Expr -> Expr
subst m (a :$ b) = (subst m a :$ subst m b)
subst m (Var n) = case Map.lookup n m of
  Just expr -> expr
  Nothing   -> (Var n)

-- Check if correct substition for a rule can be made.
tryBuildExpr :: [Pat] -> Expr -> [Expr] -> Maybe Expr
tryBuildExpr pats rule exps | length exps >= len = unflattenExpr exp'
  where
    len = length pats
    exp' = (flattenExpr (subst getMap rule)) ++ restExp
    getMap   = Map.fromList (zip pats exps)
    restExp  = drop len exps

tryBuildExpr _ _ _ = Nothing

-- Tries to apply rule at current depth in derivation tree.
tryApply :: StepFunction
tryApply defMap expr = case flattenExpr expr of
  ((Var n):xs) -> case Map.lookup n defMap of
    Nothing                 -> Nothing
    Just (Def _ pats expr') -> tryBuildExpr pats expr' xs
  _                         -> Nothing

-- Tries to make a single step of normal derivation.
rstepCasual :: StepFunction
rstepCasual defMap (Var n)  = tryApply defMap (Var n)
rstepCasual defMap (a :$ b) = case tryApply defMap (a :$ b) of
  Just c  -> Just c
  Nothing -> case rstepCasual defMap a of
    Just c  -> Just (c :$ b)
    Nothing -> case rstepCasual defMap b of
      Just c  -> Just (a :$ c)
      Nothing -> Nothing

-- Returns whole path of normal derivation for an expression.
rpath :: StepFunction -> DefMap -> Expr -> [Expr]
rpath stepFn prog expr = case stepFn prog expr of
  Just exp' -> (expr:rpath stepFn prog exp')
  Nothing   -> [expr]