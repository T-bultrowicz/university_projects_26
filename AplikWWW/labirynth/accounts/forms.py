from django import forms
from django.contrib.auth.models import User

class RegisterForm(forms.Form):
    username  = forms.CharField(max_length=150)
    email     = forms.EmailField()
    password1 = forms.CharField(widget=forms.PasswordInput, label="Password")
    password2 = forms.CharField(widget=forms.PasswordInput, label="Confirm Password")

    def clean_username(self):
        username = self.cleaned_data.get("username", "").strip()
        if len(username) < 3:
            raise forms.ValidationError("Username must be at least 3 characters long.")
        if User.objects.filter(username=username).exists():
            raise forms.ValidationError("Username is already taken.")
        return username

    def clean_email(self):
        email = self.cleaned_data.get("email", "").strip()
        if User.objects.filter(email=email).exists():
            raise forms.ValidationError("This email is already registered.")
        return email

    def clean(self):
        cleaned = super().clean()
        p1 = cleaned.get("password1")
        p2 = cleaned.get("password2")
        
        if not p1 or not p2:
            raise forms.ValidationError("Password fields cannot be empty!")      
        if p1 != p2:
            raise forms.ValidationError("Passwords do not match.")
                    
        return cleaned

    def save(self):
        data = self.cleaned_data
        return User.objects.create_user(
            username=data["username"],
            email=data["email"],
            password=data["password1"],
        )