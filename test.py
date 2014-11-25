#!/usr/bin/env python2
# -*- Mode: python -*-

import os, sys, difflib
from subprocess import Popen, PIPE, STDOUT

print 'building'

proc = Popen(['sh', './build.sh'], stdout=PIPE)
proc.communicate()
assert proc.returncode == 0, 'failed to build'

print 'testing'

for i in os.listdir('samples'):
  if i.endswith('.js'):
    print i
    expected = open(os.path.join('samples', i + '.txt')).read()
    out, err = Popen(['./a.out', os.path.join('samples', i)], stdout=PIPE).communicate()
    out = out.split('output:\n')[1]
    out = out.replace('\n\n', '\n')
    expected = expected.replace('\n\n', '\n')
    #print out
    #print '^v'
    #print expected
    assert out == expected, ''.join([a.rstrip()+'\n' for a in difflib.unified_diff(expected.split('\n'), out.split('\n'), fromfile='expected', tofile='actual')])

print 'ok.'
