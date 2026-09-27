from django import forms
from .models import Maze, RecordedRun
import json

def checkCorrectness(structure):
    # If it's a string, try to parse it as JSON
    if isinstance(structure, str):
        try:
            structure = json.loads(structure)
        except json.JSONDecodeError:
            return False
    
    # Check if it's a list (2D array)
    if not isinstance(structure, list):
        return False
    
    # Check if it's not empty
    if len(structure) == 0:
        return False
    
    # Check if all elements are lists (rows)
    if not all(isinstance(row, list) for row in structure):
        return False
    
    # Check if all rows are non-empty and contain valid numbers (0-127)
    for row in structure:
        if len(row) == 0:
            return False
        for cell in row:
            if not isinstance(cell, int) or cell < 0 or cell > 127:
                return False
    
    return True

class MazeForm(forms.ModelForm):
    class Meta:
        model = Maze
        fields = ['name', 'structure']
    
    def clean_name(self):
        name = self.cleaned_data.get('name')
        if not name:
            raise forms.ValidationError("Name cannot be empty.")
        if len(name) < 3 or len(name) > 100:
            raise forms.ValidationError("Name must be between 3 and 100 characters.")
        return name
    
    def clean_structure(self):
        structure = self.cleaned_data.get('structure')
        if not structure:
            raise forms.ValidationError("Structure cannot be empty.")
        if not checkCorrectness(structure):
            raise forms.ValidationError("Invalid maze structure.")
        
        return structure

class RecordedRunForm(forms.ModelForm):
    class Meta:
        model = RecordedRun
        fields = ['time']
