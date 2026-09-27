# Tymoteusz Bultrowicz - tb469198 - homework no. 4

import pandas as pd
import numpy as np

from sklearn.preprocessing import StandardScaler
from sklearn.neighbors import KNeighborsClassifier
from sklearn.tree import DecisionTreeClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.naive_bayes import GaussianNB
from sklearn.ensemble import RandomForestClassifier, StackingClassifier
from sklearn.ensemble import AdaBoostClassifier
from sklearn.ensemble import VotingClassifier
from sklearn.ensemble import BaggingClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import confusion_matrix
import matplotlib.pyplot as plt
from sklearn.metrics import accuracy_score, recall_score, f1_score
from sklearn.svm import SVC
import xgboost as xgb
from sklearn.metrics import accuracy_score


df = pd.read_csv('https://grantmlong.com/data/titanic.csv')
test_df = pd.read_csv('https://grantmlong.com/data/titanic_test.csv')

def get_cabin(str_cabin):
  str_cabin = str(str_cabin)
  if str_cabin == "nan":
    return "no_cabin"
  elif str_cabin[0] == 'A':
    return "A"
  elif str_cabin[0] == 'B':
    return "B"
  elif str_cabin[0] == 'C':
    return "C"
  else:
    return "D"

# cleaning out data uncorrelating with survival
df = df.drop(['PassengerId', 'Name', 'Ticket'], axis=1)

# adding dummy columns and fixing missing ages
df['Cabin'] = df['Cabin'].map(get_cabin)
df['Sex'] = df['Sex'].map({"male":0, "female":1})
df = pd.get_dummies(df, columns=['Pclass', 'Cabin', 'Embarked'])
df['Age'] = df['Age'].map(lambda x: 100 if np.isnan(x) else x)


# scaling standing-out age & fare values
scaler = StandardScaler()
scaler.fit(df[['Age', 'Fare']])
df[['Age', 'Fare']] = scaler.transform(df[['Age', 'Fare']])

# prepare army of algorithms to compare
base_learners = [
                 ('rf_1', RandomForestClassifier(n_estimators=10, random_state=42)),
                 ('rf_2', KNeighborsClassifier(n_neighbors=5))
                ]

xgb_clf = xgb.XGBClassifier(
    objective='binary:logistic',
    eval_metric='logloss',
    n_estimators=100,
    learning_rate=0.1,
    max_depth=4
)

bag_clf = BaggingClassifier(
    DecisionTreeClassifier(),
    n_estimators=500,
    max_samples=100,
    bootstrap=True, # this is bagging, set bootstrap=False to use pasting
    n_jobs=-1,
)

pasting_clf = BaggingClassifier(
    DecisionTreeClassifier(),
    n_estimators=500,
    max_samples=100,
    bootstrap=False, # this is pasting
    n_jobs=-1,
)

hard_voter_clf = VotingClassifier(
    estimators=[('rf', RandomForestClassifier()), ('knn', KNeighborsClassifier(n_neighbors=5)),
                ('svc', SVC())],
    voting='hard')
soft_voter_clf = VotingClassifier(
    estimators=[('rf', RandomForestClassifier()), ('knn', KNeighborsClassifier(n_neighbors=5)),
                ('lr', LogisticRegression())],
    voting='soft')

# here our tables of desired classifiers
single_clfs = ([
  ('knn', KNeighborsClassifier(n_neighbors=5)),
  ('naive bayes', GaussianNB()),
  ('support vector', SVC()),
  ('decision tree', DecisionTreeClassifier(max_depth=15)),
  ('logistic regression', LogisticRegression())
])

ensemble_clfs = ([
  ('adaboost', AdaBoostClassifier(
    DecisionTreeClassifier(max_depth=5), n_estimators=200,
    learning_rate=0.5)),
  ('stacking', StackingClassifier(estimators=base_learners, final_estimator=SVC())),
  ('xgBoost', xgb_clf),
  ('random forest', RandomForestClassifier(n_estimators=200, max_leaf_nodes=16, n_jobs=-1)),
  ('bagging', bag_clf),
  ('pasting', pasting_clf),
  ('hard voting clf', hard_voter_clf),
  ('soft voting clf', soft_voter_clf)
])

# splitting data into training and testing
test = df['Survived'].copy()
df = df.drop(['Survived'], axis=1)
x_train, x_test, y_train, y_test = train_test_split(df, test, test_size=0.2, random_state=42)

all_clfs = single_clfs + ensemble_clfs
results = []

for name, clf in all_clfs:
  clf.fit(x_train, y_train)
  y_pred = clf.predict(x_test)
  print(f"{name} reached accuracy_score of {accuracy_score(y_test, y_pred):.4f}\n")
  acc = accuracy_score(y_test, y_pred)
  rec = recall_score(y_test, y_pred)
  f1 = f1_score(y_test, y_pred)
  results.append({
      'Model': name,
      'Accuracy': acc,
      'Recall': rec,
      'F1-Score': f1
  })

results_df = pd.DataFrame(results)
results_df = results_df.sort_values(by='F1-Score', ascending=True)

# Drawing the plot
ax = results_df.set_index('Model').plot(
    kind='bar',
    figsize=(15,7),
    width=0.7,
    colormap='viridis'
)

plt.title('Comparing models')
plt.xlabel('Model - used')
plt.ylabel('Metric value')

# Some tricks borrowed from Chat Gpt to upgrade visibility
plt.xticks(rotation=45, ha='right')
plt.ylim(0, 1.1)
plt.yticks([float(i) / 10 for i in range(1,11)])
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.legend(loc='upper right', framealpha=1.0)
plt.tight_layout()

# Showing the plot
plt.show()
