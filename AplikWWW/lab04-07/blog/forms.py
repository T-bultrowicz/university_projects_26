from django import forms
from django.utils.text import slugify
from .models import Comment, Post, Category

class CommentForm(forms.ModelForm):
    class Meta:
        model  = Comment
        fields = ["author", "email", "body"]
        widgets = {
            "body": forms.Textarea(attrs={"rows": 4}),
        }
        labels = {
            "author": "Your Name",
        }

    def clean_author(self):
        author = self.cleaned_data["author"]
        author = author.strip()
        if len(author) < 2:
            raise forms.ValidationError("Author name must be at least 2 characters long.")
        return author

    def clean_body(self):
        body = self.cleaned_data["body"]
        if len(body) > 1000:
            raise forms.ValidationError("Comment body must be at most 1000 characters long.")

        return body
    
class PostForm(forms.ModelForm):
    class Meta:
        model = Post
        fields = ["title", "body", "category"]
        widgets = {
            "body": forms.Textarea(attrs={"rows": 8}),
        }
        labels = {
            "title": "Post Title",
        }
    
    def clean_title(self):
        title = self.cleaned_data["title"]
        title = title.strip()
        if Post.objects.filter(title=title).exists():
            raise forms.ValidationError("A post with this title already exists.")
        return title

    def clean_category(self):
        category = self.cleaned_data["category"]
        if category is None:
            raise forms.ValidationError("Please add a category.")
        if not Category.objects.filter(pk=category.pk).exists():
            raise forms.ValidationError("Selected category does not exist.")
        return category
    
    def save(self, commit=True):
        post = super().save(commit=False)
        post.slug = slugify(post.title)
        if commit:
            post.save()
        return post