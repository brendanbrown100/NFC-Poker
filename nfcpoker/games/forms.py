from django import forms
from .models import Game


class GameForm(forms.ModelForm):
    class Meta:
        model = Game
        fields = ['name', 'player_number', 'game_data']  # ⬅️ CHANGED: removed profit, added player_number
        widgets = {
            'name': forms.TextInput(attrs={
                'class': 'form-control',
                'placeholder': 'Enter game name (optional)'  # ⬅️ UPDATED placeholder
            }),
            'player_number': forms.NumberInput(attrs={  # ⬅️ NEW FIELD
                'class': 'form-control',
                'placeholder': 'Enter your player number (e.g., 1, 2, 3...)',
                'min': '1',
                'max': '6'
            }),
            'game_data': forms.FileInput(attrs={
                'class': 'form-control',
                'accept': '.txt',
                'required': True  # ⬅️ ADDED required
            })
        }
        labels = {
            'name': 'Game Name (Optional)',  # ⬅️ UPDATED label
            'player_number': 'Your Player Number',  # ⬅️ NEW label
            'game_data': 'Game Data File (.txt) *'  # ⬅️ UPDATED label
        }
        
    def __init__(self, *args, **kwargs):  # ⬅️ NEW METHOD
        super().__init__(*args, **kwargs)
        # Make name field not required
        self.fields['name'].required = False
        # Make game_data required
        self.fields['game_data'].required = True