from setuptools import find_packages
from setuptools import setup

setup(
    name='tdt_interface',
    version='0.0.0',
    packages=find_packages(
        include=('tdt_interface', 'tdt_interface.*')),
)
