# Hist/Inputs.py

# Directory where skimmed files will be stored
eosHistDir = "root://hip-cms-se.csc.fi/store/user/rverma/diff-jerc-json/Hist" 

# Years and Channels to process
Years = [
    '2024'
]

Channels = [
    'ZeeJet'
]

# VOMS Proxy path (adjust as needed)
vomsProxy = "x509up_u93032"

# Events per job
eventsPerJobMC = 1e6  # Number of events per job for MC
eventsPerJobData = 5e6  # Number of events per job for Data

