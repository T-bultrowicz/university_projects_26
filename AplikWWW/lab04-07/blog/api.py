import json
from django.http import JsonResponse
from django.views import View
from django.views.decorators.csrf import csrf_exempt
from django.utils.decorators import method_decorator
from django.shortcuts import render
from .models import Post, Comment
from .forms import PostForm, CommentForm

def post_to_dict(post):
    return {
        "id":       post.id,
        "title":    post.title,
        "slug":     post.slug,
        "body":     post.body,
        "pub_date": post.pub_date.isoformat(),
        "category": post.category.name if post.category else None,
    }

@method_decorator(csrf_exempt, name="dispatch")
class PostListView(View):
    def get(self, request):
        posts = Post.objects.all()
        return JsonResponse({"posts": [post_to_dict(p) for p in posts]})

    def post(self, request):
        if not request.user.is_authenticated:
            return JsonResponse({"error": "Authentication required."}, status=401)

        try:
            data = json.loads(request.body)
        except json.JSONDecodeError:
            return JsonResponse({"error": "Invalid JSON."}, status=400)

        form = PostForm(data)
        if not form.is_valid():
            return JsonResponse({"error": "Invalid form data.", "details": form.errors}, status=400)
        post = form.save()

        return JsonResponse({"post": post_to_dict(post)}, status=201)

@method_decorator(csrf_exempt, name="dispatch")
class PostDetailView(View):

    def get_post(self, pk):
        try:
            return Post.objects.get(pk=pk)
        except Post.DoesNotExist:
            return None

    def get(self, request, pk):
        post = self.get_post(pk)
        if post is None:
            return JsonResponse({"error": "Not found."}, status=404)
        return JsonResponse(post_to_dict(post))

    def patch(self, request, pk):
        if not request.user.is_authenticated:
            return JsonResponse({"error": "Authentication required."}, status=401)
        post = self.get_post(pk)
        if post is None:
            return JsonResponse({"error": "Not found."}, status=404)

        try:
            data = json.loads(request.body)
        except json.JSONDecodeError:
            return JsonResponse({"error": "Invalid JSON."}, status=400)

        allowed = ["title", "slug", "body"]
        for field in allowed:
            new_val = data.get(field)
            if new_val is not None:
                setattr(post, field, new_val)

        post.save()
        return JsonResponse({"post": post_to_dict(post)})

    def delete(self, request, pk):
        if not request.user.is_authenticated:
            return JsonResponse({"error": "Authentication required."}, status=401)
        
        post = self.get_post(pk)
        if post is not None:
            post.delete()
            return JsonResponse({"message": "Deleted."}, status =204)
    
        return JsonResponse({"message": "Post not found."}, status=204)
    
@method_decorator(csrf_exempt, name="dispatch")
class PostCommentView(View):
    """GET/POST comments for a given post."""

    def get(self, request, pk):
        post = Post.objects.filter(pk=pk).first()
        if post is None:
            return JsonResponse({"error": "Not found."}, status=404)
        comments = post.comments.filter(active=True)
        data = [
            {
                "id":      c.id,
                "author":  c.author,
                "body":    c.body,
                "created": c.created.isoformat(),
            }
            for c in comments
        ]
        return JsonResponse({"comments": data})

    def post(self, request, pk):
        post = Post.objects.filter(pk=pk).first()
        if post is None:
            return JsonResponse({"error": "Post Not found."}, status=404)
        
        try:
            data = json.loads(request.body)
        except json.JSONDecodeError:
            return JsonResponse({"error": "Invalid JSON."}, status=400)
        
        form = CommentForm(data)
        if not form.is_valid():
            return JsonResponse({"error": "Invalid form data.", 
                                 "details": form.errors}, status=400)
        
        comment = form.save(commit=False)
        comment.post = post
        comment.save()

        return JsonResponse({
            "comment": {
                "post":    comment.post.title,
                "author":  comment.author,
                "email":   comment.email,
                "body":    comment.body,
                "created": comment.created.isoformat(),
            }
        }, status=201)