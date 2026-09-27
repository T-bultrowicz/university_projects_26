# Tymoteusz Bultrowicz - tb469198 - homework 2 - netflix KNN recomendation
# Open using .ipynb colab file

# Task 1 solution
!curl -o test.zip https://files.grouplens.org/datasets/movielens/ml-100k.zip
!unzip test.zip
# !head ml-100k/u.data
# !cat ml-100k/u.info

import pandas as pd
from sklearn.preprocessing import MinMaxScaler
from sklearn.neighbors import NearestNeighbors

# can tweak to switch K values
K_PARAM = 20
FILMS_TO_RETURN = 5
MOVIE_KIND_WEIGHT = 10

#parsing input data
ratingDF = pd.read_csv("ml-100k/u.data", sep="\t", encoding="utf-8", header=None, names=["user", "movie", "rating", ""])
userDF = pd.read_csv("ml-100k/u.user", sep="|", encoding="utf-8", header=None, names=["id", "age", "gender", "profession", ""])
filmDF = pd.read_csv("ml-100k/u.item", sep="|", encoding="latin1", header=None)
printDF = filmDF.copy()
ratingDF = ratingDF.iloc[:, range(3)]
userDF = userDF.iloc[:, range(4)]

cols = [0]
# skipping unnecessary data from movie title to dummy for "unknown" film type
# (although could be important we leave film year out)
for i in range (6,24): cols.append(i)
filmDF = filmDF.iloc[:, cols]

filmDF.rename(mapper={0:"id"}, axis=1, inplace=True)

#removing NaN rows from data frames
ratingDF.dropna(inplace=True)
userDF.dropna(inplace=True)
filmDF.dropna(inplace=True)

#converting gender to bool like value, and profession into dummy zero/one value columns
userDF["gender"] = (userDF["gender"] == "F").astype(int)
userDF = pd.get_dummies(userDF, columns=["profession"], dtype=int)
# transforming ages into [0,1] range to match the rest of values
scaler = MinMaxScaler()
userDF[["age"]] = scaler.fit_transform(userDF[["age"]])

#running query that adds to userDF columns giving types of movie genres on which he left positive reviews
likedMovies = ratingDF[ratingDF["rating"] > 3]
likedMovies = likedMovies.merge(right=filmDF, left_on="movie", right_on="id")

# getting profile of liked film types for user - review "5" counts as double
getSums = likedMovies[likedMovies["rating"] == 5].groupby("user")[[i for i in range(6,24)]].sum() * 2
getSums += likedMovies[likedMovies["rating"] == 4].groupby("user")[[i for i in range(6,24)]].sum()
# convert sum into percentage distribution
getSums = getSums.div(getSums.sum(axis=1), axis=0).fillna(0)
# make movie kinds more important for cosine similarity
getSums *= MOVIE_KIND_WEIGHT

# add this info to userDF
userDF = userDF.merge(getSums, left_on="id", right_index=True)
userDF.columns = userDF.columns.astype(str)

# prepare model for acquiring nearest neighbours:
near_neighbors_model = NearestNeighbors(n_neighbors=K_PARAM, metric="cosine")
near_neighbors_model.fit(userDF.iloc[:, 1:]) # IMPORTANT do not include id in train!


# gets film recommendations for user following given hyper params
def getFilmRecommendation(user, userDF, filmDF, ratingDF, model):
  global FILMS_TO_RETURN
  # find closest users
  distances, indices = model.kneighbors(user.iloc[:,1:])

  # run query to get all films others liked, which we didn't like
  films_to_rate = ratingDF[ratingDF["user"].isin(indices[0,1:])]
  films_watched = ratingDF[ratingDF["user"].isin([indices[0,0]])]
  watched_movie_ids = films_watched["movie"].unique()
  films_to_rate = films_to_rate[~films_to_rate["movie"].isin(watched_movie_ids)]

  # WEIGHT WE ADD TO A SCORE WITH EACH REVIEW
  weights = {1:-1, 2:0, 3:0, 4:1, 5:2}

  # calculating scores
  similarity_map = {indices[0, i]: (1 - distances[0, i]) for i in range(len(indices[0]))}
  user_distance = films_to_rate["user"].map(similarity_map)
  rating_weights = films_to_rate["rating"].map(weights)
  films_to_rate["score"] = user_distance * rating_weights


  # return FILMS_TO_RETURN films with best score
  result = (films_to_rate.groupby("movie")["score"].sum()
    .sort_values(ascending=False)
    .head(FILMS_TO_RETURN))

  return result

# prettyPrints single user recommendation result
def prettyPrintRecommendation(user, userDF, filmDF, ratingDF, model):
  global printDF
  values = pd.DataFrame(getFilmRecommendation(user, userDF, filmDF, ratingDF, model))
  print(f"For user of id {int(user["id"].iloc[0])} our proposition is: \n\n")
  i = 1
  for index, row in values.iterrows():
    movie_title = printDF.loc[printDF[0] == index, 1].values[0]
    print(f"At spot {i} is movie {movie_title} with rating of {row.iloc[0]:.3f}")
    i += 1

  print("\n\n")

for user_id in range(5,75,5):
  user = userDF[userDF["id"] == user_id]
  if len(user) >= 1:
   prettyPrintRecommendation(user, userDF, filmDF, ratingDF, near_neighbors_model)