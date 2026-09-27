from django.contrib import admin
from .models import Maze, RecordedRun

# Register your models here.
@admin.register(Maze)
class MazeAdmin(admin.ModelAdmin):
    list_display = ["name", "structure"]

@admin.register(RecordedRun)
class RecordedRunAdmin(admin.ModelAdmin):
    list_display = ["user", "maze", "time"]