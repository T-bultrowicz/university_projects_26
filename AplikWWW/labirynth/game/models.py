from django.db import models
from django.contrib.auth.models import User

# Create your models here.
class Maze(models.Model):
    name = models.CharField(max_length=100)
    structure = models.JSONField()

    def __str__(self):
        return self.name

class RecordedRun(models.Model):
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    maze = models.ForeignKey(Maze, on_delete=models.CASCADE)
    time = models.IntegerField()  # milliseconds
    date = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return f"{self.user.username} - {self.maze.name} - {self.time:.3f}s"