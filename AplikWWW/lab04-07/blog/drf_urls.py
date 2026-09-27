from rest_framework.routers import DefaultRouter
from .drf_views import PostViewSet

router = DefaultRouter()
router.register("posts", PostViewSet)

urlpatterns = router.urls