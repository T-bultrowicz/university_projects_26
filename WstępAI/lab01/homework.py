# Tymoteusz Bultrowicz - 1 praca domowa
# to run in .ipynb

import matplotlib as plt
import numpy as np
import pandas as pd
import plotly.express as px

# EXERCISE 14

# importing data
!pip install gdown
!gdown https://drive.google.com/uc?id=1GW1pjKOCoKOlC4Jqbqql_ghYD_n0iC6O
# !head "BDL municipality incomes 2015-2020.csv"
income = pd.read_csv("BDL municipality incomes 2015-2020.csv", sep=";", dtype={"Code": "str"})  # note the explicit type specification (codes have leading zeros)

# let us first translate codes to voivodeship names
voivodeship_names = {
    "02": "Dolnośląskie",
    "04": "Kujawsko-pomorskie",
    "06": "Lubelskie",
    "08": "Lubuskie",
    "10": "Łódzkie",
    "12": "Małopolskie",
    "14": "Mazowieckie",
    "16": "Opolskie",
    "18": "Podkarpackie",
    "20": "Podlaskie",
    "22": "Pomorskie",
    "24": "Śląskie",
    "26": "Świętokrzyskie",
    "28": "Warmińsko-mazurskie",
    "30": "Wielkopolskie",
    "32": "Zachodniopomorskie"
}
code_list = [s[:2] for s in income["Code"]]
name_list = [voivodeship_names[code] for code in code_list]
income["Voivodeship"] = name_list

fig = px.histogram(income[income["2020"] < 3e07],
             x="2020",
             facet_col="Voivodeship",
             facet_col_wrap=4,
             labels={"2020": "Income in PLN"},
             title="Income of Polish municipalities in 2020 per voivodeship - fragment")
fig.for_each_annotation(lambda a: a.update(text=a.text.split("=")[-1]))
fig.show()

# EXERCISE 18

fig = px.scatter(income[income["2019"] < 5e7],
                 x = "2019",
                 y = "2020",
                 hover_data = "Voivodeship",
                 hover_name = "Region",
                 title ='Income of Polish municipalities in 2020 vs 2019')
fig.add_scatter(x=[0,5e7], y=[0,5e7], showlegend=False)
fig.show()