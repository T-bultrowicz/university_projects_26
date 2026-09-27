module DeriveE.Rpath (
    module DeriveE.Definitions,
    fromHsModule,
    showDeriveProcess
) where

import DeriveE.Helpers
import DeriveE.Definitions
import DeriveE.Reduce
import DeriveE.FromHs
import Language.Haskell.Syntax

maxSteps :: Int
maxSteps = 30

fromHsModule :: HsModule -> Either Err Prog
fromHsModule hsMod = checkProg $ fromHsModuleUnchecked hsMod

showDeriveProcess :: Prog -> (String -> String)
showDeriveProcess p = prettyExprs (map beback resCut)
  where
    resCut       = take maxSteps res
    res          = rpath rstepCasual defMap (findMainExpr defMap)
    defMap       = buildDefMap pUni
    beback       = renameBack  mUni
    (mUni, pUni) = renameUnique p