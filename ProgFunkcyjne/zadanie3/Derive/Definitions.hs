module Derive.Definitions where

type Err = String
type Name = String
data Prog = Prog { progDefs :: [Def] }
data Def = Def { defMatches :: [Match] }
data Match = Match
    { matchName :: Name
    , matchPats :: [Pat]
    , matchRhs  :: Expr
    }

infixl 9 :$
data Expr
    = Var Name
    | Con Name
    | Expr :$ Expr
data Pat = PVar Name | PApp Name [Pat]

instance Show Prog where
  showsPrec _ (Prog defs) = foldr (.) id (map shows defs)

instance Show Def where
  showsPrec _ (Def mtchs) = foldr (.) id (map shows mtchs)

instance Show Match where
  showsPrec _ (Match n ps e) =
    showString n . 
    showChar ' ' .
    (foldr (.) id (map (showsPrec 0) ps)) .
    showString "= " .
    (showsPrec 0 e) .
    showChar '\n'

instance Show Pat where
  showsPrec 0 n = (showsPrec 1 n) . showChar ' '
  showsPrec _ (PVar n) = showString n
  showsPrec _ (PApp n []) = showString n
  showsPrec _ (PApp n ps) =
    showChar '(' . 
    showString n . 
    showChar ' ' . 
    (concatPs ps) .
    showChar ')'
    where
      concatPs [] = id
      concatPs (p:[]) = showsPrec 1 p
      concatPs (p:pss) = showsPrec 0 p . concatPs pss

instance Show Expr where
  showsPrec _ (Var n) = showString n
  showsPrec _ (Con n) = showString n
  showsPrec 0 (e1 :$ e2) =
    showsPrec 0 e1 .
    showChar ' ' .
    showsPrec 1 e2

  showsPrec _ (e1 :$ e2) =
    showChar '(' .
    (showsPrec 0 (e1 :$ e2)) .
    showChar ')'

getName :: Def -> Name
getName = matchName . head . defMatches