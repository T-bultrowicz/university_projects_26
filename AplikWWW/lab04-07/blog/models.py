from django.db import models

# Create your models here.
class Category(models.Model):
    name = models.CharField(max_length=100)
    slug = models.SlugField(unique=True)

    def __str__(self):
        return self.name

class Post(models.Model):
    title    = models.CharField(max_length=200)
    slug     = models.SlugField(unique=True)
    body     = models.TextField()
    view_count = models.PositiveIntegerField(default=0)
    pub_date = models.DateTimeField(auto_now_add=True)
    category = models.ForeignKey(Category, on_delete=models.SET_NULL,
                                 null=True, blank=True, related_name="posts")

    class Meta:
        ordering = ["-pub_date"]   # newest first

    def __str__(self):
        return self.title

class Comment(models.Model):
    post    = models.ForeignKey(Post, on_delete=models.CASCADE,
                                related_name="comments")
    author  = models.CharField(max_length=100)
    email   = models.EmailField()
    body    = models.TextField()
    created = models.DateTimeField(auto_now_add=True)
    active  = models.BooleanField(default=True)

    class Meta:
        ordering = ["created"]

    def __str__(self):
        return f"Comment by {self.author} on {self.post}"