# Tymoteusz Bultrowicz - tb469198 - homework no. 6

!kaggle datasets download zalando-research/fashionmnist
!unzip fashionmnist.zip

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.decomposition import PCA
from sklearn.metrics import adjusted_rand_score
from sklearn.metrics import silhouette_score
from sklearn.cluster import KMeans
from sklearn.cluster import AgglomerativeClustering
from sklearn.manifold import LocallyLinearEmbedding
from sklearn.preprocessing import StandardScaler
from scipy.cluster.hierarchy import dendrogram, linkage

# We need to reduce size of train data, as our calculations are to heavy.
# As we use train data only to set our hyper-parametres, it is acceptable.

print("Beginning! Reading CSV file! \n")
train_data = pd.read_csv('fashion-mnist_train.csv')
train_data = train_data.head(5000)

X_train = train_data.drop('label', axis=1)
y_train = train_data['label']

test_data = pd.read_csv('fashion-mnist_test.csv')
X_test = test_data.drop('label', axis=1)
y_test = test_data['label']

# Scaling data
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
for_final_test_pca = PCA(n_components = 95).fit(X_train)
X_test = scaler.transform(X_test)


# Our own dim reduction by merging pixels together and removing edges.
# 784 pixels -> 144 pixels. Optimalization done by ChatGPT,
# as I wouldn't come up with "tricks" for optimalization, rather would use "for" loops.
def pixel_cutter(input):
  x, y = input.shape
  input = input.reshape(x, 14, 2, 14, 2).mean(axis=(2, 4))
  mask = np.ones((14, 14), dtype=bool)
  mask[:, 0] = False
  mask[:, -1] = False
  mask[-1, :] = False
  mask[0, :] = False
  input = input[:, mask]

  return input.reshape(x, -1)

# Our function to generate descriptions
def generate_description(method_dim, dim, method, distance):
  return (method_dim + ("" if str(dim) == "" else ", dim = " + str(dim)) +
  ", method = " + method + ("" if distance == "" else ", distance = " + distance))

# we analyze dimensiality reduction variance_ratio distribution
pca_model = PCA()
pca_model.fit(X_train)
cumsum = np.cumsum(pca_model.explained_variance_ratio_)

# Drawing plot, points at 91%, 93% and 95% of total variance.
print("Plot of explained_variance in PCA() - this is how number of dimensions left corresponds to variance saved.\n")
plt.figure(figsize=(8, 6))
plt.scatter(130, cumsum[130], color='red')
plt.scatter(190, cumsum[190], color='green')
plt.scatter(95, cumsum[95], color='purple')
plt.plot(cumsum)
plt.xlabel('Number of components')
plt.ylabel('% of total covariance')
plt.show()

# Beginning testing hyper-parameters:
#  - method_dim : PCA or pixel_cutter
#  - dim: only for PCA, how much n_components left
#  - method: k_means or agglomerative
#  - distance: euclidean, cosine, manhattan - self explanatory, only for agglomerative
# n_cluster set constantly to 10 as there are 10 kinds of labels.
print("\nBeginning testing of different hyper params (check comments in code). It will take around 1-1,5mins\n")

results = []
for method_dim in ["PCA", "pixel_cutter"]:
  if method_dim == "PCA":
    for dim in [95, 130, 190]:
      X_dim_train = PCA(n_components=dim).fit_transform(X_train)
      for method in ["k_means", "agglomerative"]:
        if method == "k_means":
          km_model = KMeans(n_clusters=10).fit(X_dim_train)
          results.append([
              generate_description(method_dim, dim, method, ""),
              adjusted_rand_score(y_train, km_model.labels_),
              silhouette_score(X_dim_train, km_model.labels_)])
        else:
          for distance in ["euclidean", "cosine", "manhattan"]:
            ag_model = AgglomerativeClustering(n_clusters=10, metric=distance,
            linkage=("ward" if distance == "euclidean" else "average")).fit(X_dim_train)

            results.append([
                generate_description(method_dim, dim, method, distance),
                adjusted_rand_score(y_train, ag_model.labels_),
                silhouette_score(X_dim_train, ag_model.labels_)])
  else:
    X_dim_train = pixel_cutter(X_train)
    for method in ["k_means", "agglomerative"]:
        if method == "k_means":
          km_model = KMeans(n_clusters=10).fit(X_dim_train)
          results.append([
              generate_description(method_dim, "", method, ""),
              adjusted_rand_score(y_train, km_model.labels_),
              silhouette_score(X_dim_train, km_model.labels_)])
        else:
          for distance in ["euclidean", "cosine", "manhattan"]:
            ag_model = AgglomerativeClustering(n_clusters=10, metric=distance,
            linkage=("ward" if distance == "euclidean" else "average")).fit(X_dim_train)

            results.append([
                generate_description(method_dim, "", method, distance),
                adjusted_rand_score(y_train, ag_model.labels_),
                silhouette_score(X_dim_train, ag_model.labels_)])

results = pd.DataFrame(results, columns=["Method", "ARI", "Silhouette"])
print("Results ready!!\n")

# Nice graph of results from test datasets. Made with help of ChatGPT.
results_sorted = results.sort_values(by='ARI', ascending=True)
labels = results_sorted['Method'].values
ari_vals = results_sorted['ARI'].values
sil_vals = results_sorted['Silhouette'].values

x = np.arange(len(labels))  # location of labels
width = 0.35                # width of single bar

fig, ax = plt.subplots(figsize=(14, 8))

rects1 = ax.bar(x - width/2, ari_vals, width, label='ARI', color='orange', edgecolor='black')
rects2 = ax.bar(x + width/2, sil_vals, width, label='Silhouette', color='blue', edgecolor='black')

ax.set_ylabel('Wynik (Score)')
ax.set_title('Wyniki ARI i Silhouette dla testset, len = #5000 (posortowane po ARI rosnąco)')
ax.set_xticks(x)
ax.set_xticklabels(labels, rotation=75)
ax.legend()

ax.grid(axis='y', linestyle='--', alpha=0.7)

plt.tight_layout()
plt.show()

print("\nBeginning performing validation test, ~20 sec on 10000 objects")

# Testing our best chosed hyper parametres on validation dataset, for final results.
X_test_dim = for_final_test_pca.transform(X_test)

ag_model = AgglomerativeClustering(n_clusters=10).fit(X_test_dim) # ward + euclidean

print("For validation dataset, using agglomerative with ward + euclidean & PCA(dim = 95), we achieve: ")
print(f"ARI: {adjusted_rand_score(y_test, ag_model.labels_):.4f}")
print(f"Silhouette: {silhouette_score(X_test_dim, ag_model.labels_):.4f}")

print("\n At last we try to show hierarchy for small sample using dendrogram! ")
helper_dist = {
    0 : "T-shirt",
    1 : "Trouser",
    2 : "Pullover",
    3 : "Dress",
    4 : "Coat",
    5 : "Sandal",
    6 : "Shirt",
    7 : "Sneaker",
    8 : "Bag",
    9 : "Ankle_boot"
}

X_sample = X_test_dim[:100]
y_sample = y_test[:100].values

Z = linkage(X_sample, method='ward')
plt.figure(figsize=(16, 6))
plt.title("Dendrogram - small probe")
dendr = dendrogram(Z, labels=[helper_dist[i] for i in y_sample], leaf_rotation=90, leaf_font_size=10)
plt.xlabel("Label of product")
plt.ylabel("Distance")
plt.show()

print("Now checking extras - LLE model - please wait a bit:")

# Extras - LLE model

lle = LocallyLinearEmbedding(n_components=10, n_neighbors=10)
X_reduced = lle.fit_transform(X_train)
ag_model2 = AgglomerativeClustering(n_clusters=10).fit(X_reduced) # ward + euclidean

print("For validation dataset, using agglomerative with ward + euclidean & PCA(dim = 95), we achieve: ")
print(f"ARI: {adjusted_rand_score(y_train, ag_model2.labels_):.4f}")
print(f"Silhouette: {silhouette_score(X_reduced, ag_model2.labels_):.4f}")

# Commented out for 

# lle = LocallyLinearEmbedding(n_components=25, n_neighbors=10)
# X_reduced = lle.fit_transform(X_train)
# ag_model2 = AgglomerativeClustering(n_clusters=10).fit(X_reduced) # ward + euclidean

# print("For validation dataset, using agglomerative with ward + euclidean & PCA(dim = 95), we achieve: ")
# print(f"ARI: {adjusted_rand_score(y_train, ag_model2.labels_):.4f}")
# print(f"Silhouette: {silhouette_score(X_reduced, ag_model2.labels_):.4f}")
# lle = LocallyLinearEmbedding(n_components=50, n_neighbors=10)
# X_reduced = lle.fit_transform(X_train)
# ag_model2 = AgglomerativeClustering(n_clusters=10).fit(X_reduced) # ward + euclidean

# print("For validation dataset, using agglomerative with ward + euclidean & PCA(dim = 95), we achieve: ")
# print(f"ARI: {adjusted_rand_score(y_train, ag_model2.labels_):.4f}")
# print(f"Silhouette: {silhouette_score(X_reduced, ag_model2.labels_):.4f}")
# lle = LocallyLinearEmbedding(n_components=100, n_neighbors=10)
# X_reduced = lle.fit_transform(X_train)
# ag_model2 = AgglomerativeClustering(n_clusters=10).fit(X_reduced) # ward + euclidean

# print("For validation dataset, using agglomerative with ward + euclidean & PCA(dim = 95), we achieve: ")
# print(f"ARI: {adjusted_rand_score(y_train, ag_model2.labels_):.4f}")
# print(f"Silhouette: {silhouette_score(X_reduced, ag_model2.labels_):.4f}")

