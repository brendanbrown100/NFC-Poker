from django.db import models
from django.contrib.auth.models import User


class Game(models.Model):
    id = models.AutoField(primary_key=True)
    user = models.ForeignKey(User, on_delete=models.CASCADE, related_name='games')
    name = models.CharField(max_length=255, blank=True, null=True)  # ⬅️ NOW OPTIONAL
    player_number = models.IntegerField()  # ⬅️ NEW REQUIRED FIELD
    date = models.DateTimeField(auto_now_add=True)
    profit = models.IntegerField(default=0)
    game_data = models.FileField(upload_to='game_files/')  # ⬅️ NOW REQUIRED (removed blank=True, null=True)
    created_at = models.DateTimeField(auto_now_add=True)
    
    class Meta:
        ordering = ['-created_at']
    
    def __str__(self):
        game_name = self.name if self.name else f"Game #{self.id}"  # ⬅️ UPDATED
        return f"{game_name} - Player {self.player_number} - {self.user.username}"  # ⬅️ UPDATED