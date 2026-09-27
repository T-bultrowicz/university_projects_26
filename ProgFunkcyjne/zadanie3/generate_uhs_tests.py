import os

def create_uhs_file(filename, content):
    """Tworzy plik o rozszerzeniu .uhs w folderze examples."""
    os.makedirs("examples", exist_ok=True)
    filepath = os.path.join("examples", filename)
    with open(filepath, "w") as f:
        f.write(content.strip() + "\n")
    print(f"Wygenerowano: {filepath}")

# Test 1: Głębokie dopasowanie i currying (Ackermann)
# Sprawdza czy ewaluator radzi sobie z wielokrotnym zagnieżdżeniem wywołań w argumencie.
ackermann = """
-- Funkcja Ackermanna - klasyk spowalniający ewaluatory
-- Oczekiwane dla ack one two (czyli A(1,2)) to S (S (S Z)) (czyli 4, na liczbach Peano S Z = 1)
-- UWAGA: Może wyssać bardzo dużo "paliwa" (fuel)!
ack Z n = S n
ack (S m) Z = ack m (S Z)
ack (S m) (S n) = ack m (ack (S m) n)

one = S Z
two = S one

main = ack one two
"""

# Test 2: Niestandardowe aplikacje jako argumenty
# Testuje poprawność ewaluatora, gdy do funkcji przekazana jest bezpośrednia złożona aplikacja.
complex_main = """
-- Złożone struktury zagnieżdżonych drzew aplikacji w 'main'
id x = x
apply f x = f x
const x y = x
compose f g x = f (g x)

main = compose id (const Foo) (apply id Bar)
"""

# Test 3: Listy zaimplementowane na konstruktorach i dopasowanie do drugiego argumentu
# Weryfikuje, czy `tryMatch` dobrze omija pierwszy argument i skupia się na drugim 
# oraz testuje rekursję na długich listach.
list_operations = """
-- data List = Nil | Cons x List
-- tail
tl Nil = Nil
tl (Cons x xs) = xs

-- append (łączenie list)
append Nil ys = ys
append (Cons x xs) ys = Cons x (append xs ys)

-- lista [Foo, Bar, Baz]
listA = Cons Foo (Cons Bar Nil)
listB = Cons Baz Nil

main = tl (append listA listB)
"""

# Test 4: Leniwa ewaluacja z nieskończonymi listami (Lazy Evaluation check)
# Testuje czy model zdejmuje tylko tyle, ile wymaga redukcja.
# head() powinno wyjąć pierwszy element ze z nieskończonej listy 'infList' bez zawieszenia.
lazy_infinite = """
-- nieskończona lista samych Z
infList = Cons Z infList

hd (Cons x xs) = x

-- Funkcja map na liście
map f Nil = Nil
map f (Cons x xs) = Cons (f x) (map f xs)

inc (S x) = S (S x)
inc Z = S Z

main = hd (map inc infList)
"""

# Test 5: Combinator logic explosion (S, K, I) z wymuszeniem zagnieżdżeń
# Bardzo rygorystyczny test na zachowanie kręgosłupa zippera (spine).
combinators = """
-- Combinatory
s x y z = x z (y z)
k x y = x
i x = x

-- M powinno wykonać głębokie propagowanie S
main = s (s k i) (s (k k) i) Foo Bar
"""

# Test 6: Rozbudowany Pattern Matching - Omijanie niewłaściwych klauzul
# Wymaga precyzyjnego przejścia przez wszystkie niedopasowane wzorce, aby trafić w 'catch-all'.
deep_patterns = """
-- Wyszukiwanie specyficznego drzewa
-- Jeśli ewaluator "utknie" w złym stanie, nie trafi do czwartej klauzuli
foo (S (S Z)) A = X
foo (S Z) B = Y
foo (S x) C = Z
foo x y = Fail

main = foo (S (S (S Z))) B
"""

# Test 7: Wielokrotne przekazywanie funkcji jako argumentu (Higher Order Function)
# Złożona struktura 'main' z 5+ aplikacjami bez typowego pattern matchingu na danych, a na funkcjach.
hof_test = """
twice f x = f (f x)
wrap x = Box x

-- wywoła wrap(wrap(wrap(wrap(wrap(wrap(wrap(wrap(Foo))))))))
main = twice twice twice wrap Foo
"""

# Test 8: Zagnieżdżona struktura boolowska i liczby z dużym naddatkiem apliakcji w RHS
# Sprawdza złożone RHS zawierające mnóstwo aplikacji.
bool_logic = """
not True = False
not False = True

and True True = True
and x y = False

isZero Z = True
isZero (S x) = False

main = and (not (isZero (S Z))) (isZero Z)
"""

def generate():
    create_uhs_file("ackermann.uhs", ackermann)
    create_uhs_file("complex_main.uhs", complex_main)
    create_uhs_file("list_operations.uhs", list_operations)
    create_uhs_file("lazy_infinite.uhs", lazy_infinite)
    create_uhs_file("combinators.uhs", combinators)
    create_uhs_file("deep_patterns.uhs", deep_patterns)
    create_uhs_file("hof_test.uhs", hof_test)
    create_uhs_file("bool_logic.uhs", bool_logic)

if __name__ == "__main__":
    print("Generowanie zaawansowanych testów .uhs...")
    generate()
    print("Gotowe. Pliki znajdują się w folderze 'examples/'.")