# Tymoteusz Bultrowicz - tb469198 - Homework 5

import numpy as np
import matplotlib.pyplot as plt
import plotly.express as px

from sklearn.cluster import KMeans
from scipy.cluster.hierarchy import dendrogram
from sklearn.cluster import DBSCAN
from sklearn.cluster import AgglomerativeClustering
from sklearn.datasets import load_iris
from sklearn.metrics import silhouette_score
from sklearn.metrics import calinski_harabasz_score

# MADE ANALYZING TO COMPLICATED
# # makes 3d plotting from every possible 3set of dimensions of X
# # that is coloured into clusters using labels.
# # works only if X is four dimensional.
# def make_plotting(X, labels, title_name):
#   for i in range(4):
#     fig = px.scatter_3d(x=X[:, (i + 1) % 4], y=X[:, (i + 2) % 4], z=X[:, (i + 3) % 4], color=labels)
#     fig.update_layout(title=title_name)
#     fig.show()

# make simple plots, chatGPT helped with better display
def make_simple_plotting(X, labels, title_name, descrip_name):
  fig = px.scatter_3d(x=X[:, 0], y=X[:, 1], z=X[:, 2], color=labels)
  fig.update_layout(
      title=title_name,
      width=400,
      height=400,
      margin=dict(l=0, r=0, b=0, t=40),
      scene=dict(
          xaxis_title=descrip_name[0],
          yaxis_title=descrip_name[1],
          zaxis_title=descrip_name[2]
      )
  )
  fig.show()

iris = load_iris()
X = iris.data

# Doing so many plots caused some to fail with rendering when I was testing it.
# So if some plotting does not load, comment out the rest and run script again.
print("Beginning model training process!!\n")

summaries = []
summaries.append('\nKmean models: \n')

# K_mean models testing
for k in range(2, 10):
  model = KMeans(n_clusters=k)
  model.fit(X)
  ss = silhouette_score(X, model.labels_)
  ch = calinski_harabasz_score(X, model.labels_)
  summaries.append(f"K_mean model with k = {k} achieved silhouette score of {ss:.3f} and calinski harabasz score of {ch:.3f}\n")
  make_simple_plotting(X, model.labels_, f"K_mean model with k = {k}", iris.feature_names)

summaries.append('\nAgglomerative models: \n')
# Agglomerative models testing
for link in ['ward', 'complete', 'average', 'single']:
  for metric in ['euclidean', 'cosine']:
    if link == 'ward' and metric == 'cosine':
      continue
    model = AgglomerativeClustering(n_clusters=3, linkage=link, metric=metric)
    model = model.fit(X)
    ss = silhouette_score(X, model.labels_)
    ch = calinski_harabasz_score(X, model.labels_)
    summaries.append(f"Agglomerative model with linkage='{link}', metric='{metric}' achieved silhouette score of {ss:.3f} and calinski harabasz score of {ch:.3f}\n")
    make_simple_plotting(X, model.labels_, f"Agglomerative model with linkage='{link}', metric='{metric}'", iris.feature_names)

# DBScan models testing

summaries.append('\nDBScan models: \n')
for eps in range(3, 7):
  for min_samples in range(2, 10, 2):
    model = DBSCAN(eps=float(eps) / 10, min_samples=min_samples)
    model.fit(X)
    ss = silhouette_score(X, model.labels_)
    ch = calinski_harabasz_score(X, model.labels_)
    summaries.append(f"DBScan model with eps='{(float(eps) / 10):.3f}', min_samples='{min_samples}' achieved silhouette score of {ss:.3f} and calinski harabasz score of {ch:.3f}\n")
    make_simple_plotting(X, model.labels_, f"DBScan model with eps='{(float(eps) / 10):.3f}', min_samples='{min_samples}'", iris.feature_names)


# scores of tests
for i in summaries:
  print(i)

