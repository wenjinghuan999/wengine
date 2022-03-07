def import_or_install(package, install_package_name=None):
    import importlib
    import pip
    try:
        importlib.import_module(package)
    except ImportError:
        from pip._internal import main
        main(['install', '--user', install_package_name or package])
    finally:
        globals()[package] = importlib.import_module(package)