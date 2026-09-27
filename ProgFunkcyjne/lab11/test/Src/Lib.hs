module Src.Lib where

type NEL a = (a, [a])

headNEL :: NEL a -> a
headNEL (x, _) = x

tailNEL :: NEL a -> NEL a
tailNEL (x, (xp:xs)) = (xp, xs)
tailNEL (x, []) = error "Would create empty list"

lastNEL :: NEL a -> a
lastNEL (x, []) = x
lastNEL (x, (xp:xs)) = last (xp:xs)

initNEL :: NEL a -> NEL a
initNEL (x, (xp:xs)) = (x, init (xp:xs))
initNEL (x, []) = error "Would create empty list"

addNEL :: NEL a -> NEL a -> NEL a
addNEL (a, as) (b, bs) = (a, concat [as, [b], bs])

class (NEL a) where
  arbitrary = (arbitrary :: a, (arbitrary :: Gen[a]))
  shrink t = [t]
