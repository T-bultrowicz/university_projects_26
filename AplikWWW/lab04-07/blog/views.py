from django.shortcuts import render, get_object_or_404, redirect
from django.contrib.auth.decorators import login_required, user_passes_test
from .models import Post, Category, Comment
from django.db.models import Prefetch, F
from .forms import CommentForm, PostForm

def post_list(request):
    posts      = Post.objects.select_related("category")
    categories = Category.objects.all()
    return render(request, "blog/post_list.html", {
        "posts": posts, "categories": categories,
    })

def post_detail(request, slug):
    post     = get_object_or_404(Post.objects.prefetch_related(
        Prefetch("comments", queryset=Comment.objects.filter(active=True), to_attr="active_comments")
    ), slug=slug)
    comments = post.active_comments
    form     = CommentForm()

    if request.method == "POST":
        form = CommentForm(request.POST)
        if form.is_valid():
            comment      = form.save(commit=False)
            comment.post = post
            comment.save()
            return redirect("blog:post_detail", slug=slug)
    elif request.method == "GET":
        Post.objects.filter(pk=post.pk).update(view_count=F("view_count") + 1)

    return render(request, "blog/post_detail.html", {
        "post": post, "comments": comments, "form": form,
    })

@login_required
@user_passes_test(lambda u: u.is_staff)
def post_create(request):
    if request.method == "POST":
        form = PostForm(request.POST)
        if form.is_valid():
            new_post = form.save()
            return redirect("blog:post_detail", slug=new_post.slug)
    else:
        form = PostForm()

    return render(request, "blog/post_create.html", {"form": form})

def category_posts(request, slug):
    category = get_object_or_404(Category, slug=slug)
    posts = Post.objects.filter(category=category)
    return render(request, "blog/post_list.html", {
        "posts": posts,
        "categories": Category.objects.all(),
        "active_category": category,
    })