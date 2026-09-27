from django.shortcuts import render, redirect
from django.contrib.auth import login
from django.contrib.auth.decorators import login_required
from django.db.models import ExpressionWrapper, Min, F, Q, FloatField
from django.db.models.functions import Cast

from .forms import RegisterForm
from game.models import Maze

def register(request):
    if request.method == "POST":
        form = RegisterForm(request.POST)
        if form.is_valid():
            user = form.save()
            login(request, user, backend="django.contrib.auth.backends.ModelBackend")
            return redirect('game:home')
    else:
        form = RegisterForm()
    return render(request, "accounts/register.html", {"form": form})

@login_required
def profile(request):
    user = request.user
    mazes = list(Maze.objects.annotate(
        best_time_ms=Min(
            'recordedrun__time',
            filter=Q(recordedrun__user=user)
        )
    ).order_by('name'))

    for maze in mazes:
        if maze.best_time_ms is not None:
            maze.best_time = maze.best_time_ms / 1000.0
        else:
            maze.best_time = None

    context = {
        "user": user,
        "mazes": mazes
    }
    return render(request, "accounts/profile.html", context)