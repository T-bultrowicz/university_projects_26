from django.urls import path
from .api import MazeListView, MazeDetailView, RecordedRunListView, RecordedRunDetailView

urlpatterns = [
    path('mazes/', MazeListView.as_view(), name='api_maze_list'),
    path('mazes/<int:pk>/', MazeDetailView.as_view(), name='api_maze_detail'),
    path('runs/', RecordedRunListView.as_view(), name='api_run_list'),
    path('runs/<int:pk>/', RecordedRunDetailView.as_view(), name='api_run_detail'),
    path('mazes/<int:pk>/runs/', RecordedRunListView.as_view(), name='api_maze_runs'),
]