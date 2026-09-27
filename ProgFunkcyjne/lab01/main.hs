s x y z = x z (y z)
k x y = x
i x = x
b x y z = x (y z)

zero f x = x
suc g f x = f (g f x)
add h g = (h suc g)
mul h g = (h (add g) zero)
expo h g = g h

magic x y | mod x y == 0 = "good"
          | otherwise = "not good"

two = suc (suc zero)
three = suc two

runNat n = n (+1) 0
funNat n = n ('|':) ""

gPred n = mod n 10 == 6
gList = [2^n | n <- [0..]]