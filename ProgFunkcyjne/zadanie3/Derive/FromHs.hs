module Derive.FromHs (
  module Language.Haskell.Syntax,
  fromHsModule
) where
import Derive.Definitions
import Language.Haskell.Syntax
import qualified Data.Set as Set

-- Funkcja eksportowana, konwertująca moduł HsModule do mojego Prog.
fromHsModule :: HsModule -> Either Err Prog
fromHsModule x = fromHsModuleUnchecked x >>= verifyProg

-- Wnętrze HsModule, nie przeprowadzające weryfikacji.
fromHsModuleUnchecked :: HsModule -> Either Err Prog
fromHsModuleUnchecked (HsModule _ _ _ _ dls) = Prog <$> traverse fromHsDecl dls

-- Funkcje weryfikujące podaną strukturę programu.
findMain :: [Def] -> Either Err Def -- Znajdowanie pojedynczego maina programu.
findMain defs = 
  case filter (all nmMain . defMatches) defs of
    [main] -> Right main
    _ -> Left "Main was not found in file!\n"
  where
    nmMain match = (matchName match) == "main"

verifyMain :: Def -> Either Err () -- Weryfikacja prawidłowej struktury maina.
verifyMain main = oneMatch main >>= noArgs
  where
    oneMatch mn = if length (defMatches mn) == 1
      then Right (head $ defMatches mn)
      else Left "Multiple matches to main found in the file!\n"
    noArgs match = if length (matchPats match) == 0
      then Right ()
      else Left "Main with args detected in the file!\n"

-- Nie ma wielokrotnych definicji funkcji o tej samej nazwie, 
-- przyjmujących różną ilość argumentów.
noMultiple :: [Def] -> Set.Set Name -> Either Err ()
noMultiple [] _ = Right ()
noMultiple (d:ds) seen = case Set.member defName seen of
  True -> Left $ concat [
    "Multiple definitions of function ", defName, 
    " with different argument count found in the file!\n"]
  False -> noMultiple ds (Set.insert defName seen)
  where
    defName = getName d

-- Nie ma powtarzających się nazw jako zmiennych w funkcji.
noSameArgs :: [Def] -> Either Err ()
noSameArgs defs = case all everyMatchUnique defs of
  True -> Right ()
  False -> Left "Same name repeats as argument for a function in the file!\n"
  where
    everyMatchUnique = all (uniqueNames Set.empty . matchPats) . defMatches
    uniqueNames _ [] = True
    uniqueNames seen (p:ps) = 
      Set.disjoint (names p) seen && uniqueNames (Set.union (names p) seen) ps
    names (PVar n) = Set.singleton n
    names (PApp _ ps) = Set.unions (map names ps)

-- Funckja przeprowadzająca wszystkie konieczne weryfikacje.
verifyProg :: Prog -> Either Err Prog
verifyProg prog = 
  noMultiple (progDefs prog) Set.empty >>
  noSameArgs (progDefs prog) >>
  findMain (progDefs prog) >>= 
  verifyMain >> pure prog

-- Parsowanie HsDecl.
fromHsDecl :: HsDecl -> Either Err Def
fromHsDecl (HsFunBind mtchs) = Def <$> traverse fromHsMatch mtchs
fromHsDecl (HsPatBind _ pat rhs _) = Def <$> (:[]) <$> (
    Match
    <$> (fromPatName <$> fromHsPat pat)
    <*> pure []
    <*> fromHsRhs rhs
  )
fromHsDecl _ = Left "Unrecognisable definition in the file!\n"

-- Parsowanie HsMatch, HsPat, HsExp i HsRhs.
fromHsMatch :: HsMatch -> Either Err Match
fromHsMatch (HsMatch _ nam pats rhs _) = 
  Match <$> pure (fromHsName nam)
        <*> traverse fromHsPat pats
        <*> fromHsRhs rhs

fromHsName :: HsName -> Name
fromHsName (HsIdent x) = x
fromHsName (HsSymbol x) = x

fromPatName :: Pat -> Name
fromPatName (PVar x) = x
fromPatName (PApp x _) = x

fromHsPat :: HsPat -> Either Err Pat
fromHsPat (HsPVar x) = Right (PVar (fromHsName x))
fromHsPat (HsPParen p) = fromHsPat p
fromHsPat (HsPApp (UnQual x) pats) = 
  PApp <$> (pure $ fromHsName x) <*> traverse fromHsPat pats
fromHsPat _ = Left "Unexpected pattern in file!" 

fromHsExp :: HsExp -> Either Err Expr
fromHsExp (HsVar (UnQual nam)) = Right (Var $ fromHsName nam)
fromHsExp (HsCon (UnQual nam)) = Right (Con $ fromHsName nam)
fromHsExp (HsParen e) = fromHsExp e
fromHsExp (HsApp e1 e2) = (:$) <$> fromHsExp e1 <*> fromHsExp e2
fromHsExp _ = Left "Unexpected expression in file!"

fromHsRhs :: HsRhs -> Either Err Expr
fromHsRhs (HsUnGuardedRhs e) = fromHsExp e
fromHsRhs _ = Left "Guarded RHS are not supported!"