from django.shortcuts import render
from .models import Announcement


def index(request):
    template_data = {}
    template_data['title'] = 'NFC-POKER'
    return render(request, 'home/index.html', {
        'template_data': template_data})

def about(request):
    template_data = {}
    template_data['title'] = 'About'
    return render(request,
                  'home/about.html',
                  {'template_data': template_data})

def announcements(request):
    announcements = Announcement.objects.all().order_by('-created_at')
    template_data = {}
    template_data['title'] = 'Announcements'
    return render(request,
                  'home/announcements.html',
                  {'template_data': template_data,
                   'announcements': announcements})
