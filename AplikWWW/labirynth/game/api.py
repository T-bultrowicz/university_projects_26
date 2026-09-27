import json
from django.http import JsonResponse
from django.views import View
from django.views.decorators.csrf import csrf_exempt
from django.utils.decorators import method_decorator
from django.shortcuts import get_object_or_404
from urllib3 import request
from .models import Maze, RecordedRun
from .forms import MazeForm, RecordedRunForm

def maze_to_dict(maze):
    return {
        "id": maze.id,
        "name": maze.name,
        "structure": maze.structure,
    }

def run_to_dict(run):
    return {
        "id": run.id,
        "user": run.user.username if run.user else None,
        "maze": run.maze.name if run.maze else None,
        "time": run.time,
        "date": run.date.isoformat() if run.date else None,
    }

@method_decorator(csrf_exempt, name="dispatch")
class MazeListView(View):
    def get(self, request):
        mazes = Maze.objects.all()
        query = request.GET.get("q", "")
        if query:
            mazes = mazes.filter(name__icontains=query)

        return JsonResponse({"mazes": [maze_to_dict(m) for m in mazes]})

    def post(self, request):
        if not request.user.is_authenticated or not request.user.is_staff:
            return JsonResponse({"error": "Authentication and staff status required."}, status=403)

        try:
            data = json.loads(request.body)
        except json.JSONDecodeError:
            return JsonResponse({"error": "Invalid JSON."}, status=400)

        form = MazeForm(data)
        if not form.is_valid():
            return JsonResponse({"error": "Invalid form data.", "details": form.errors}, status=400)
        maze = form.save()

        return JsonResponse({"maze": maze_to_dict(maze)}, status=201)

@method_decorator(csrf_exempt, name="dispatch")
class MazeDetailView(View):
    def get_maze(self, pk):
        try:
            return Maze.objects.get(pk=pk)
        except Maze.DoesNotExist:
            return None

    def get(self, request, pk):
        maze = self.get_maze(pk)
        if maze is None:
            return JsonResponse({"error": "Not found."}, status=404)
        return JsonResponse(maze_to_dict(maze))

    def patch(self, request, pk):
        if not request.user.is_authenticated or not request.user.is_staff:
            return JsonResponse({"error": "Authentication and staff status required."}, status=403)
        maze = self.get_maze(pk)
        if maze is None:
            return JsonResponse({"error": "Not found."}, status=404)

        try:
            data = json.loads(request.body)
        except json.JSONDecodeError:
            return JsonResponse({"error": "Invalid JSON."}, status=400)

        allowed = ["name", "structure"]
        for field in allowed:
            new_val = data.get(field)
            if new_val is not None:
                setattr(maze, field, new_val)

        maze.save()
        return JsonResponse({"maze": maze_to_dict(maze)})

    def delete(self, request, pk):
        if not request.user.is_authenticated or not request.user.is_staff:
            return JsonResponse({"error": "Authentication and staff status required."}, status=403)
        
        maze = self.get_maze(pk)
        if maze is not None:
            maze.delete()
            return JsonResponse({"message": "Deleted."}, status=204)
    
        return JsonResponse({"message": "Maze not found."}, status=204)

@method_decorator(csrf_exempt, name="dispatch")
class RecordedRunListView(View):
    def get(self, request, *args, **kwargs):
        pk = kwargs.get("pk")
        runs = RecordedRun.objects.filter(maze_id=pk) if pk else RecordedRun.objects.all()
        
        query = request.GET.get("q", "")
        if query:
            runs = runs.filter(user__username__icontains=query)
            
        return JsonResponse({"runs": [run_to_dict(r) for r in runs]})

    def post(self, request, *args, **kwargs):
        if not request.user.is_authenticated:
            return JsonResponse({"error": "Unauthorized"}, status=401)
            
        try:
            data = json.loads(request.body)
        except json.JSONDecodeError:
            return JsonResponse({"error": "Invalid JSON"}, status=400)

        # Convert simple {"time": 1000} payload to form data if necessary
        time_val = data.get("time")
        if time_val is None:
            return JsonResponse({"error": "Field 'time' is required"}, status=400)

        data_for_form = {"time": time_val}
        
        form = RecordedRunForm(data_for_form)
        if not form.is_valid():
            return JsonResponse({"error": "Invalid data", "details": form.errors}, status=400)

        maze_pk = kwargs.get("pk") or data.get("maze_id")
        maze = get_object_or_404(Maze, pk=maze_pk) if maze_pk else None
        if not maze:
            return JsonResponse({"error": "Maze ID required"}, status=400)

        run = form.save(commit=False)
        run.user = request.user
        run.maze = maze
        run.save()

        return JsonResponse({"run": run_to_dict(run)}, status=201)

@method_decorator(csrf_exempt, name="dispatch")
class RecordedRunDetailView(View):
    def get_run(self, pk):
        try:
            return RecordedRun.objects.get(pk=pk)
        except RecordedRun.DoesNotExist:
            return None

    def get(self, request, pk):
        run = self.get_run(pk)
        if run is None:
            return JsonResponse({"error": "Not found."}, status=404)
        return JsonResponse(run_to_dict(run))

    def delete(self, request, pk):
        if not request.user.is_authenticated or not request.user.is_staff:
            return JsonResponse({"error": "Authentication and staff status required."}, status=403)
        
        run = self.get_run(pk)
        if run is not None:
            run.delete()
            return JsonResponse({"message": "Deleted."}, status=204)
    
        return JsonResponse({"message": "Run not found."}, status=204)