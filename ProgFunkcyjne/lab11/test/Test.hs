import Src.Lib
import Test.Tasty
import Test.Tasty.HUnit
import Test.QuickCheck

main = defaultMain $ testGroup "Unit tests" [test1, test2, test3]

test1 = testCase "NEL [1] + NEL[2] = NEL [1,2]" $ (1, [2]) @?= addNEL (1, []) (2, [])
test2 = testCase "NEL [1] + NEL [2,3] = NEL[1,2,3]" $ (1, [2, 3]) @?= addNEL (1, []) (2, [3])
test3 = testCase "lastNEL [1] = 1" $ 1 @?= lastNEL (1, [])
