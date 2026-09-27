from sklearn.datasets import load_iris
from sklearn.model_selection import train_test_split
import pandas as pd
import numpy as np

class OneRAlgorithm:
    def __init__(self, num_splits=16):
        # Hyper param - amount of splits
        self.num_splits = num_splits

        # Atributes of fitted model
        self.best_column = None # the one feature we use
        self.rules = {} # rules to transform
        self.bins = None # bins for best category
        self.default_class = None # most often occuring category

    def fit(self, X, y):
        self.default_class = y.mode()[0] # most occuring category

        min_total_errors = X.shape[0]

        # Iterating trying to find best columns
        for column in X.columns:
            feature_data = X[column].copy()
            is_continuous = False
            col_bins = None

            # Discretize numeric types.
            if pd.api.types.is_numeric_dtype(X[column]):
                if len(feature_data.unique()) > self.num_splits:
                    is_continuous = True
                    feature_data, col_bins = pd.cut(feature_data, bins=self.num_splits, labels=False, retbins=True)

            # Begin to create rules:
            df_temp = pd.DataFrame({'feature': feature_data, 'target': y})
            column_rules = {}
            column_errors = 0
            grouped = df_temp.groupby('feature')['target']

            for feature_value, target_series in grouped:
                majority_class = target_series.mode()[0]
                column_rules[feature_value] = majority_class

                # Counting errors
                errors_in_group = (target_series != majority_class).sum()
                column_errors += errors_in_group

            # Currently the best column
            if column_errors < min_total_errors:
                min_total_errors = column_errors
                self.best_column = column
                self.rules = column_rules
                self.bins = col_bins

        print(f"Chosen column: '{self.best_column}' error count: {min_total_errors}")

    def predict(self, X):
        if self.best_column is None:
            raise ValueError("Not fitted model! Firstly use .fit method! ")

        # winning column
        column_data = X[self.best_column].copy()

        # discretizing
        if self.bins is not None:
            column_data = pd.cut(column_data, bins=self.bins, labels=False, include_lowest=True)

        # results
        predictions = column_data.map(self.rules)
        predictions = predictions.fillna(self.default_class) # if sth new occurs

        return predictions.values

iris = load_iris()
iris_data = pd.DataFrame(iris.data, columns=iris.feature_names)
iris_target = pd.Series(iris.target)

X_train, X_test, y_train, y_test = train_test_split(iris_data, iris_target, test_size=0.2, random_state=42)

ora = OneRAlgorithm()
ora.fit(X_train, y_train)
y_pred = ora.predict(X_train)
print(f"Train Accuracy: {np.mean(y_pred == y_train)}")
y_pred = ora.predict(X_test)
print(f"Test Accuracy: {np.mean(y_pred == y_test)}")
