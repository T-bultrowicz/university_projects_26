-- Zadanie 2 - Tymoteusz Bultrowicz - tb469198 - 10.04.2026

module Main where
import System.Environment
import Language.Haskell.Syntax
import Language.Haskell.Parser
import DeriveE.Rpath

main :: IO ()
main = do
  args <- getArgs
  case args of
    ["--help"] -> usage
    [f] -> do
      fContent <- readFile f
      putStr (mainDerive fContent "")

    _          -> usage

mainDerive :: String -> (String -> String)
mainDerive fContent = case parseModule fContent of
  ParseOk x -> case fromHsModule x of
    Left err   -> showString err
    Right prog -> shows prog . showChar '\n' . showDeriveProcess prog

  ParseFailed errLoc errCod -> showParsingError errLoc errCod

usage :: IO ()
usage = do
  putStrLn "Usage: zadanie2 [--help] [file]"
  putStrLn "  --help  - display this message"
  putStrLn "  file    - input file"

showParsingError :: SrcLoc -> Err -> (String -> String)
showParsingError errLoc errCod =
  showString "Syntax parsing failed at line " . 
  shows (srcLine errLoc) . 
  showString ", column " . 
  shows (srcColumn errLoc) . 
  showString " due to: " . 
  showString errCod
