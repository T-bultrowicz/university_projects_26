import json
import time
from django.shortcuts import render, redirect, get_object_or_404
from django.contrib.auth.decorators import login_required, user_passes_test
from django.http import StreamingHttpResponse
from .forms import MazeForm
from .models import Maze, RecordedRun

# Create your views here.
def home(request):
    context = {
        "page_title": "main page",
        "heading": "welcome dear user",
    }
    return render(request, "game/home.html", context)

def play(request):
    return render(request, "game/play.html")

def is_staff(user):
    return user.is_staff

@login_required
@user_passes_test(is_staff)
def create_maze(request):
    if request.method == 'POST':
        form = MazeForm(request.POST)
        if form.is_valid():
            form.save()
            return redirect('game:home')
    else:
        form = MazeForm()
    
    context = {
        "page_title": "create maze",
        "heading": "Create New Maze",
        "form": form
    }
    return render(request, "game/create.html", context)

def play_game(request, game_name):
    maze = get_object_or_404(Maze, name=game_name)
    context = {
        "maze": maze
    }
    return render(request, "game/labirynth.html", context)

def top_runs_event_stream(level_id):
    while True:
        top_runs = RecordedRun.objects.filter(maze_id=level_id).order_by('time')[:5]
        data_to_send = [
            {
                "player": run.user.username,
                "time": run.time,
            }
            for run in top_runs
        ]
        yield f"data: {json.dumps(data_to_send)}\n\n"
        time.sleep(5)

def level_leaderboard_stream(request):
    if not request.user.is_authenticated:
        return StreamingHttpResponse(
            'data: {"error": "User not authenticated"}\n\n', 
            content_type="text/event-stream"
        )

    level_id = request.GET.get('level_id') 
    
    if not level_id:
        return StreamingHttpResponse(
            'data: {"error": "No level specified"}\n\n', 
            content_type="text/event-stream"
        )

    response = StreamingHttpResponse(
        top_runs_event_stream(level_id), 
        content_type="text/event-stream"
    )
    
    response["Cache-Control"] = "no-cache"
    return response