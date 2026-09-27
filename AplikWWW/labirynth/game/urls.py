from django.urls import path, include
from . import views

app_name = 'game'

urlpatterns = [
    path("", views.home, name="home"),
    path("api/", include("game.api_urls")),
    path("play/", views.play, name="play"),
    path("play/<str:game_name>/", views.play_game, name="play_game"),
    path("create/", views.create_maze, name="create_maze"),
    path("leaderboard_stream/", views.level_leaderboard_stream, name="leaderboard_stream"),
]
