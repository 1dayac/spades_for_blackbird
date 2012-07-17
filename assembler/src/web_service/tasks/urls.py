from django.conf.urls import patterns, include, url
from django.conf.urls.static import static
from django.contrib.staticfiles.urls import staticfiles_urlpatterns

urlpatterns = patterns('tasks.views',
    url(r'^$', 'index'),
    url(r'^(?P<task_id>\d+)/$', 'datails'),
    url(r'^newtask$', 'newtask'),
    url(r'^addtask$', 'addtask'),                       
    url(r'^(?P<task_id>\d+)/remove$', 'remove'),                       
    url(r'^(?P<task_id>\d+)/decrease$', 'decrease'),                       
    url(r'^(?P<task_id>\d+)/increase$', 'increase'),                                              
    url(r'^(?P<task_id>\d+)/added$', 'added'),                                              
    )

urlpatterns += staticfiles_urlpatterns()


