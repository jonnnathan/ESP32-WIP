Import("env")

# Install missed package
try:
    import intelhex
except ImportError:
    env.Execute("$PYTHONEXE -m pip install intelhex")