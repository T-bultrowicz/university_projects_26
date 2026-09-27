#Tymoteusz Bultrowicz - tb469198 - homework no. 7
!kaggle datasets download quantbruce/real-estate-price-prediction
!unzip real-estate-price-prediction.zip
!rm real-estate-price-prediction.zip
!gdown 1HX-q3FJ9uD2lpTfwWaSdYZO35dPVY4Rc

import pandas as pd
import numpy as np
from sklearn.linear_model import LinearRegression, ElasticNet, SGDRegressor, Ridge, Lasso
from sklearn.preprocessing import StandardScaler
import matplotlib.pyplot as plt

def mystery_data():
  df = pd.read_csv('Real estate.csv')
  df = df.drop(columns=['No'])

  X = df.drop(columns=['Y house price of unit area'])
  X = StandardScaler().fit_transform(X)
  y = df['Y house price of unit area']

  lr = LinearRegression()
  lr.fit(X, y)
  for i in range(1, 7):
    print(f'Coefficient of X{i} is {lr.coef_[i-1]:.4f}')

def old_cars():
  df = pd.read_csv('drive_time_sedans.csv')

  # preparing the data by getting dummies & dropping not needed columns
  df = df.drop(columns=["vehicle.age.group", "makex"])
  cat_cols = ["overage", "vehicle.type", "domestic.import", "color.set", "state", "make.model"]
  num_cols = ["lot.sale.days", "mileage", "vehicle.age"]
  df = pd.get_dummies(df, columns=cat_cols, drop_first=True)

  # splitting the data into train and validation sets, removing cost - target columm
  train = df[df["data.set"] == "TRAIN"]
  train = train.drop(columns=["data.set"])
  test = df[df["data.set"] != "TRAIN"]
  test = test.drop(columns=["data.set"])

  # scaling the data
  scaler = StandardScaler()
  scaler.fit(train[num_cols])
  train[num_cols] = scaler.transform(train[num_cols])
  test[num_cols] = scaler.transform(test[num_cols])

  # X/y split
  X_train = train.drop(columns=["total.cost"])
  y_train = train["total.cost"]
  X_test = test.drop(columns=["total.cost"])
  y_test = test["total.cost"]

  # training on train set - we prepare all hyper-params
  best_name = ""
  best_res = float("inf")
  best_model = None
  names = ["Linear Regression"]
  for alpha in ["low", "med", "high", "very_high"]:
    for name in ["Lasso", "Ridge", "Elastic 0.25 l1ratio", "Elastic 0.5 ratio", "Elastic 0.75 ratio"]:
      names.append(f"{name} - {alpha} alpha")
  for loss in ["epsilon_insensitive", "squared_error", "huber"]:
    for learning_rate in ["adapting", "constant"]:
      names.append(f"SGDRegressor - J = {loss} - Learn rate = {learning_rate}")
  
  models = [LinearRegression()]
  for alpha in [0.1, 1.0, 10.0, 100.0]:
    models.append(Lasso(alpha=alpha))
    models.append(Ridge(alpha=alpha))
    for ratio in [0.25, 0.5, 0.75]:
      models.append(ElasticNet(alpha=alpha, l1_ratio=ratio))
  for loss in ["epsilon_insensitive", "squared_error", "huber"]:
    for learning_rate in ["adaptive", "constant"]:
      models.append(SGDRegressor(loss=loss, learning_rate=learning_rate, max_iter=3000))
  results = []
  
  

  tuple_list = zip(names, models)
  for name, model in tuple_list:
    model.fit(X_train, y_train)
    pred = model.predict(X_train)
    res = np.mean((pred - y_train)**2)

    if res < best_res:
      best_name = name
      best_res = res
      best_model = model
    print(f"{name} achieved squared error of {res:.4f}")
    results.append(res)
  # validating on the best model from a group

  print(f"\nBest model from testing is {best_name} - testing it on the test set\n")
  pred = best_model.predict(X_test)
  res = np.mean((pred - y_test)**2)
  print(f"Achieved squared error of {res:.4f}")


  # # showing a diagram of all train results
  print("\n\n Showing diagram of all train results:")
  fig, ax = plt.subplots(figsize=(17, 6))
  dane_posortowane = sorted(zip(results, names), reverse=True)
  posortowane_wyniki = [x[0] for x in dane_posortowane]
  posortowane_nazwy = [x[1] for x in dane_posortowane]

  bars = ax.bar(posortowane_nazwy, posortowane_wyniki, 
              color='cornflowerblue', edgecolor='black', alpha=0.8)
  ax.bar_label(bars, fmt='%.4f', padding=3, fontsize=10)
  ax.set_title('Porównanie skuteczności modeli (Zbiór treningowy)', fontsize=14, pad=15)
  ax.set_xlabel('Model', fontsize=12)
  ax.set_ylabel('Wynik', fontsize=12)
  plt.xticks(rotation=45, ha='right', fontsize=10)
  ax.grid(axis='y', linestyle='--', alpha=0.5)
  plt.tight_layout()
  plt.show()





print("Mystery data function: \n")
mystery_data()
print("\n Old cars task: \n")
old_cars()
