# https://gist.github.com/bjuliano/5e25ee0728d454c5f88d41f69cd9016e
# Convert sklearn.Bunch to pandas.DataFrame

# Tymoteusz Bultrowicz - lab homework - task nr 2, n-fold GaussianNB()
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from sklearn import datasets

# loading data
iris = datasets.load_iris()
df = pd.DataFrame(
    data=iris.data,
    columns=iris.feature_names
)
df["target"] = pd.Series(iris.target, dtype="int32")

# splitting data into chunks
n = int(input("Provide #n for n-folds"))
df["Chunk"] = np.random.randint(1, n+1, len(df))

# helper - different kind of irises propabilities
prop = [df[df["target"] == a].count()["target"] / df.count()["target"] for a in range(3)]

#gaussian density function
def gauss(x, mu, sigma):
  return (1 / np.sqrt(2 * np.pi * sigma ** 2)) * np.exp(-((x - mu) ** 2) / (2 * sigma ** 2))

# for collecting results
true_pos = np.array([0, 0, 0])
true_neg = np.array([0, 0, 0])
false_pos = np.array([0, 0, 0])
false_neg = np.array([0, 0, 0])

# training phases - test on chunk nr i
for i in range(n):
  # calculate means and stds for each kind of flower in training sets
  means = df[df["Chunk"] != i+1].groupby("target").mean().drop("Chunk", axis = 1)
  stds = df[df["Chunk"] != i+1].groupby("target").std().drop("Chunk", axis = 1)

  # going through samples, calculating likelihood scores
  for sample in df[df["Chunk"] == i+1].drop("Chunk", axis = 1).itertuples(index=False):
    predicted = 0
    result = -np.inf

    # assuming result for being in class nr. j
    for j in range(3):
      res = np.log(prop[j])

      # for each parameter
      for k in range(4):
        res += np.log(gauss(sample[k], means.iloc[j, k], stds.iloc[j, k]))

      if res > result:
        result = res
        predicted = j

    # checking if we predicted well (sample[4] == actual type)
    if sample[4] == predicted:
      true_pos[sample[4]] += 1
      true_neg[(sample[4] + 1) % 3] += 1
      true_neg[(sample[4] + 2) % 3] += 1
    else:
      false_neg[sample[4]] += 1
      false_pos[predicted] += 1
      true_neg[3 - sample[4] - predicted] += 1
  #exiting for sample
# exiting all loop

# calculating results accumulated from all folds
precision = true_pos / (true_pos + false_pos)
recall    = true_pos / (true_pos + false_neg)
accuracy  = (true_pos + true_neg) / (true_pos + true_neg + false_pos + false_neg)
f1_score  = 2 * precision * recall / (precision + recall)

# printing all results
print(f"True positives: {true_pos}")
print(f"True negatives: {true_neg}")
print(f"False positives: {false_pos}")
print(f"False negatives: {false_neg}")

results = pd.DataFrame({
    "precision": precision,
    "recall": recall,
    "accuracy": accuracy,
    "f1_score": f1_score
})

results.plot(kind="bar", title="Accuracy of model", xlabel="Flower types")
plt.show()
