from django.contrib import admin
from .models import Game


@admin.register(Game)
class GameAdmin(admin.ModelAdmin):
    list_display = ('id', 'name', 'user', 'player_number', 'profit', 'date', 'created_at')  # ⬅️ ADDED player_number
    list_filter = ('user', 'player_number', 'date', 'created_at')  # ⬅️ ADDED player_number
    search_fields = ('name', 'user__username')
    readonly_fields = ('date', 'created_at')
    ordering = ('-created_at',)
    
    fieldsets = (
        ('Game Information', {
            'fields': ('user', 'name', 'player_number', 'profit')  # ⬅️ ADDED player_number
        }),
        ('Game Data', {
            'fields': ('game_data',)
        }),
        ('Timestamps', {
            'fields': ('date', 'created_at'),
            'classes': ('collapse',)
        }),
    )