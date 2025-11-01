from django.urls import path
from . import views

urlpatterns = [
    path('', views.index, name='games.index'),
    path('add/', views.add_game, name='games.add'),
    path('<int:game_id>/', views.view_game, name='games.view'),
    path('<int:game_id>/delete/', views.delete_game, name='games.delete'),
]