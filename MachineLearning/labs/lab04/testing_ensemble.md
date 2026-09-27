# Summary of results
# Tymoteusz Bultrowicz - tb469198 - homework 4

# General

Speaking generally from the first look we can see that ensemble models tend 
to have higher accuracy than simple ones. Cost of that is visible longer working time for those algorithms.

# Top results

Going down to individual models we can see that SVC() model was definetely the 
best one for these task - reaching results in top 3 on every run of tests. 
Top spot was taken most often by soft voting algorithm which used as params:
    - LogisticRegression()
    - RandomForestClassifier()
    - KNeighborsClassifier()

Honorable mentions goes to bagging, xgBoost & adaBoost - other top scorers. (although a bit inconsistent)

# Disappointments

Definetely the worst algorithm to use was the GaussianNB(), maybe due to my way of 
preparing the data using dummy values.
Surprisingly the Random Forest algorithm that was taught on lecture to be one of the most accurate, 
wasn't scoring high in the trials.
