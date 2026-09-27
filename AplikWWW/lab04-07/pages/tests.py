from django.test import TestCase

class PagesTests(TestCase):

    def test_home_status(self):
        response = self.client.get("/")
        assert response.status_code == 200

    def test_about_uses_correct_template(self):
        response = self.client.get("/about/")
        self.assertTemplateUsed(response, "pages/about.html")

    def test_about_has_four_skills(self):
        response = self.client.get("/about/")
        assert len(response.context["skill_list"]) == 4

    def test_greet_contains_name(self):
        response = self.client.get("/greet/Alice/")
        assert str(response.content).find("Alice") != -1

    def test_projects_search_filters(self):
        response = self.client.get("/projects/?q=python")
        for i in response.context["projects"]:
            assert i["lang"] == "Python" or i["name"].lower.find("python") != -1

