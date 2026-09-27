module DeriveE.Definitions where
import qualified Data.Map as Map

data Def = Def Name [Pat] Expr
data Expr = Var Name | Expr :$ Expr
type Pat = Name
type Name = String
type Err = String
newtype Prog = Prog {progDefs :: [Def]}

type DefMap = Map.Map Name Def
type RenameMap = Map.Map Name Name
type ExprssMap = Map.Map Name Expr

instance Show Expr where
  showsPrec _ (a :$ (b :$ c)) = shows a . showString " (" . shows (b :$ c) . showString ")"
  showsPrec _ (a :$ b)        = shows a . showChar ' ' . shows b
  showsPrec _ (Var n)         = showString n

instance Show Def where
  showsPrec _ (Def n p e) = shS n . (foldr (.) id (map shPt p)) . shS " = " . shows e . shLn
    where
      shPt x = shSp . (shS x)
      shS = showString
      shLn = showChar '\n'
      shSp = showChar ' '

instance Show Prog where
  showsPrec _ prog = foldr (.) id (map shows (progDefs prog))

extractName :: Def -> Name
extractName (Def n _ _) = n

extractExpr :: Def -> Expr
extractExpr (Def _ _ expr) = expr

-- Build map that links definitions inside prog with their names.
buildDefMap :: Prog -> DefMap
buildDefMap p = Map.fromList (zip (map extractName defs) defs)
  where
    defs = progDefs p

