# Summary - Tymoteusz Bultrowicz - tb469198 - homework 6

## Hyper parametres - train set
I've decided to test different setpus of models and their hyper parameters on train set,
then checking general accuracy using best model from train set on test set. Model
here refers to a pair of (dim_reduction_model, clustering_model).

Due to fact that data set of 60000 objects is to computationally heavy 
for casual Agglomerative clustering, I've used only 5000 objects for training models.

## Dimensionality reduction
Here I've tested PCA as it is generally very efficient model. For n_component params of PCA
I've decided to pick number of dimensions that would preserve 91, 93 and 95% of variance. Reason
for that was their "sweet spot" placement on graph curve. For only a bit higher variance
preservation I would have to leave much more dimensions. On the other hand dropping only a few
dimensions would make score drop significantly.

Besides that I also tested own dimension reduction algorithm - glueing 4 pixels into one using mean of pixels.
It performed generally a bit worse then PCA(), however surprisingly was giving better result combined with
agglomerative clustering with 'average' linking, 'manhattan' or 'cosine' dist functions.
Reason for picking this algorithm is it's simplicity, the fact that it has very simple logical explanation,
and curiosity of how it would perform.

## Algorithms for clustering.
Here I tested the two most common techniques available in SkicitLearn. KMeans and Agglomerative with
different linking techniques.

I didn't have to experiment with different k-values, as I knew Fashion MNIST has 10 labels from 0 to 9.

Dendrogram showed, that Agglomerative clusterer behaves really promisingly on this data. But also shows flaws.
Generally items of same cathegory was really close to each other. But small percentage of the was really
far away form their cluster, sometimes on completely different side of a tree.

## Scoring - pt1. - compare to given labels
Here I decided to choose ARI algorithm, as this algorithm is designed specifically for my purpose. It is fair
and also compares your model score to score of random clusterer. Besides that algorithms that compare just 
singular values of labels vs their predicted labels would be useless, as [0,0,1,1] against [1,1,0,0] would
give scre of 0, despite it being perfectly fine clustering.

## Scoring - pt2. - scoring using shape
Here I used another universal, shown on labs algorithm - silhouette score.

## Scoring - pt3. - how models performed
Generally poorly :(
I belive reason for that was that just subtle differeces caused objects to land into different labels, even
if most of the bits holded the same value. As my clustering models / dimension reduction models
generally relied on some casual distance functions, they had no option too adjust to that situation.

Best results was capped around only 0.3 - 0.4 for AR score and 0.15 for silhouette score.
The best models where using euclidean distance measueres. The best where ward agglomerative methods, and
besides them KMeans. Shockingly to me very often PCA preserving less dimensions (PCA 91% - 95 dims)
was giving better results.

## Conclusions & Extras
Due to poor performance I tried to add another dimension reduction tool to my toolkit - Locally Linear Embedding.
Tested on the same train_test it achieved better results that the others, due to reasons shown in paragraph above:
AR around 0.40 - 0.45, much better silhoutte of 0.47 - (with n_components of 10)

However it is worth noting working time was noticeably higher for these group of test units.
