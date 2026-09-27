module Examples where
import Syntax


{-
s x y z = x z (y z)
k x y = x
main = s k k x
-}
progSKK :: Prog
progSKK =
  Prog { progDefs =
    [ Def "s" ["x","y","z"] ((Var "x" :$ Var "z") :$ (Var "y" :$ Var "z"))
    , Def "k" ["x","y"] (Var "x") 
    , Def "main" [] (((Var "s" :$ Var "k") :$ Var "k") :$ Var "x")
    ]     
  }


{-
s x y z = x z (y z)
k x y = x
i = s k k
om x = x x
omega = om om
main = k i omega Z
-}
progKIO :: Prog
progKIO =
  Prog { progDefs =
    [ Def "s" ["x","y","z"] ((Var "x" :$ Var "z") :$ (Var "y" :$ Var "z"))
    , Def "k" ["x","y"] (Var "x")
    , Def "i" [] ((Var "s" :$ Var "k") :$ Var "k"),Def "om" ["x"] (Var "x" :$ Var "x")
    , Def "omega" [] (Var "om" :$ Var "om")
    , Def "main" [] (((Var "k" :$ Var "i") :$ Var "omega") :$ Var "Z")
    ]
  }

{-
zero f z = z
one f z = f z
two = suc one
tre = suc two
suc n f z = f (n f z)
o f g x = f (g x)
add m n f x = m f (n f x)
mul m n = o m n
fyr = add two two
six = mul two tre
main = six S Z
-}
progPeano :: Prog
progPeano =
  Prog { progDefs =
    [ Def "zero" ["f","z"] (Var "z")
    , Def "one" ["f","z"] (Var "f" :$ Var "z")
    , Def "two" [] (Var "suc" :$ Var "one")
    , Def "tre" [] (Var "suc" :$ Var "two")
    , Def "suc" ["n","f","z"] (Var "f" :$ ((Var "n" :$ Var "f") :$ Var "z"))
    , Def "o" ["f","g","x"] (Var "f" :$ (Var "g" :$ Var "x"))
    , Def "add" ["m","n","f","x"] ((Var "m" :$ Var "f") :$ ((Var "n" :$ Var "f") :$ Var "x"))
    , Def "mul" ["m","n"] ((Var "o" :$ Var "m") :$ Var "n")
    , Def "fyr" [] ((Var "add" :$ Var "two") :$ Var "two")
    , Def "six" [] ((Var "mul" :$ Var "two") :$ Var "tre")
    , Def "main" [] ((Var "six" :$ Var "S") :$ Var "Z")
    ]
  }
