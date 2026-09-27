module DeriveE.FromHs where
import Language.Haskell.Syntax
import DeriveE.Definitions

-- pushes error up out of list, if one occurs
sequenceEither :: [Either Err a] -> Either Err [a]
sequenceEither []             = Right []
sequenceEither (Left err : _) = Left err
sequenceEither (Right x : xs) = case sequenceEither xs of
  Right xs'  -> Right (x : xs')
  Left err -> Left err

-- parses module into our def of Prog.
fromHsModuleUnchecked :: HsModule -> Either Err Prog
fromHsModuleUnchecked (HsModule _ _ _ _ x) = case sequenceEither (map getSingleDef x) of
  Right val  -> Right (Prog val)
  Left err -> Left err

-- simplifies single definition
getSingleDef :: HsDecl -> Either Err Def
getSingleDef (HsPatBind _ hspat hsrhs _) = getDefPatBind hspat hsrhs
getSingleDef (HsFunBind [match])         = getDefFunBind match
getSingleDef (HsFunBind [])              = Left "Empty function bind\n"
getSingleDef (HsFunBind _)               = Left "Multiple function bindings!\n"
getSingleDef _                           = Left "Unexpected definition in file!\n"

getNameFromNam :: HsName -> Name
getNameFromNam (HsIdent y)  = y
getNameFromNam (HsSymbol y) = y

getNameFromPat :: HsPat -> Either Err Name
getNameFromPat (HsPVar y) = Right (getNameFromNam y)
getNameFromPat _          = Left "Unexpected pattern in file!\n"

-- gets single definition from a function binding
getDefFunBind :: HsMatch -> Either Err Def
getDefFunBind (HsMatch _ hsnam hspats hsrhs _) = 
  case sequenceEither (map getNameFromPat hspats) of
    Right pats -> case getExpr hsrhs of
      Right expr -> Right (Def (getNameFromNam hsnam) pats expr)
      Left err -> Left err
    Left err -> Left err

-- gets single definition from a pattern binding
getDefPatBind :: HsPat -> HsRhs -> Either Err Def
getDefPatBind hspat hsrhs = case getNameFromPat hspat of
  Right nam -> case getExpr hsrhs of
    Right expr -> Right (Def nam [] expr)
    Left  err  -> Left err
  Left err  -> Left err

-- invokes recurce
getExpr :: HsRhs -> Either Err Expr
getExpr (HsUnGuardedRhs e) = getExprRec e
getExpr _                  = Left "Unexpected RHS in file! (Guards not supported)\n"

-- gets expression
getExprRec :: HsExp -> Either Err Expr
getExprRec (HsVar (UnQual hsnam)) = Right (Var (getNameFromNam hsnam))
getExprRec (HsCon (UnQual hsnam)) = Right (Var (getNameFromNam hsnam))
getExprRec (HsParen e)            = getExprRec e

getExprRec (HsApp e1 e2)          = case getExprRec e1 of
  Right x  -> case getExprRec e2 of
    Right y    -> Right (x :$ y)
    Left err -> Left err
  Left err -> Left err

getExprRec _                      = Left "Unexpected expression in file!\n"