from django.http import HttpResponse
from django.shortcuts import render
import datetime

def home(request):
    context = {
        "page_title": "main page",
        "heading": "welcome dear user",
        "server_time": datetime.datetime.now()
    }
    return render(request, "pages/home.html", context)

def about(request):
    context = {
        "page_title": "main page",
        "heading": "welcome dear user",
        "skill_list": ["Python", "HTTP", "HTML", "CSS"]
    }
    return render(request, "pages/about.html", context)

def greet(request, name):
    message = ""
    if request.method == "POST":
        message = request.POST.get("note")
    context = {
        "page_title": "main page",
        "user_name": name,
        "server_time": datetime.datetime.now(),
        "message" : message
    }
    return render(request, "pages/greet.html", context)

ALL_PROJECTS = [
    {"name": "Socket Server",    "lang": "Python",     "year": 2025, "done": True},
    {"name": "HTML Profile",     "lang": "HTML",       "year": 2025, "done": True},
    {"name": "CSS Layout",       "lang": "CSS",        "year": 2025, "done": True},
    {"name": "Django App",       "lang": "Python",     "year": 2025, "done": False},
    {"name": "REST API",         "lang": "Python",     "year": 2024, "done": True},
    {"name": "React Dashboard",  "lang": "JavaScript", "year": 2024, "done": True},
    {"name": "SQL Queries Lab",  "lang": "SQL",        "year": 2024, "done": True},
    {"name": "CLI Tool",         "lang": "Python",     "year": 2023, "done": True},
]

def projects(request):
    q = request.GET.get("q", "")
    filtered_projects = []
    if q != "":
        for i in ALL_PROJECTS:
            if i["name"].lower().find(q.lower()) != -1 or i["lang"].lower().find(q.lower()) != -1:
                filtered_projects.append(i)
    else:
        filtered_projects = ALL_PROJECTS

    done_count = 0
    for i in filtered_projects:
        if i["done"]:
            done_count += 1
    
    context = {
        "projects": filtered_projects,
        "done_count": done_count,
        "q": q
    }
    return render(request, "pages/projects.html", context)

ENTRIES = []
def guestbook(request):
    context = {}
    ENTRIES.append({"name": request.POST.get("name"), "message" : request.POST.get("message")})
    print(ENTRIES)
    return render(request, "pages/guestbook.html", context)

def api_demo(request):
    return render(request, "pages/api_demo.html")

def blog_browser(request):
    return render(request, "pages/blog_browser.html")