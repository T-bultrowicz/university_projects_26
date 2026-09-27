# Tymoteusz Bultrowicz - tb469198 - Homework 5 - summary

## KMeans
KMeans models adapted really well to data given in iris set. Highest score was scored with k=3
which perfectly aligns with 3 targets in Iris dataset. The bigger the k, the more scores in test were dropping.

## Agglomerative models
As I was testing them only with no of clusters set to 3, they had unfair advantage against other models.
Even though they didn't use it to the fullest - the best result achieved with 'ward' linkage technique was close,
but no greater than KMeans result. All other linkage method was behaving pretty similarly as long as 'euclidean'
metric was used, which performed better than 'cosine'.

## DBScan models
Propably due to low point density in Iris dataset, they were scoring very poorly. Bigger scores was achieved
only with higher values of eps, but then again it just split data into two clusters, with lots of aliens standing out.

## Summary
For this dataset KMeans is definetely the best. There are no big groups of outliers which are usual struggle for KMeans,
and distances between each data point are evenly distributed, which favor the KMeans method. This technique is also
lighter on computations than constructing dendrogram.