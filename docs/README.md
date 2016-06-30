
### Filling out the MOLASSES Configuration File:

There are a few variables that can be used or ignored to create different kinds of MOLASSES models. The kind of simulation you want to run and your desired outputs will dictate which variables you set and in what way. Follow this guide to choose the parameters you want to use in the configuration file.

 1. [Requirements for different model scenarios](#scenarios)
 1. [Tables of Parameters and whether to use them](#summarytables)
 1. [How the INITIALIZE (Full) Module chooses parameters](#initializemodule)

### <a name="scenarios"></a>Using Parameters for Given Scenarios
When a lava flow simulation is run (through the PULSE and DISTRIBUTE modules), the model needs to know 
 1. the elevation map
 2. the residual flow thickness
 3. the location of the source vents
 4. the total volume of the flow
 5. and the amount of volume to pulse at every code loop.

These five properties must be set in modules leading up to the actual simulation (e.g. FLOW\_INIT, DEM\_LOADER, INITIALIZE). If multiple simulations are desired, to make a probabilistic result, the configuration file should be set up differently than if a single deterministic flow is desired. The _INITIALIZE_ module will read in the configuration file and determine the parameters to use or ignore depending on a combination of parameters. For instance, if a vent location (VENT\_EASTING and VENT\_NORTHING) is given in the configuration file, the INITIALIZE module will not pass on the VENT\_SPATIAL\_DENSITY\_FILE parameter, even if it is set.

#### Running a single lava flow
The INITIALIZE module will use parameters for a single, deterministic lava flow if
 1. The NEW\_VENT parameter is given at least once and
 2. SIMULATIONS is set to 1, or SIMULATIONS is absent from the configuration file

To run a single flow with set parameters you'll need to use the following input parameters at a minimum:
 * DEM\_FILE
 * RESIDUAL\_THICKNESS
 * NEW\_VENT
 * VENT\_EASTING
 * VENT\_NORTHING
 * VENT\_PULSE\_VOLUME
 * VENT\_TOTAL\_VOLUME and
 * at least one Output file format

#### Running a probabilistic model of a flow with known vent location(s)
The INITIALIZE module will use parameters for a probabilistic flow model from one or more known vent locations if
 1. The NEW\_VENT parameter is given at least once and
 2. SIMULATIONS is set to something greater than 1.
 
To run this kind of flow model, use the following:
 * DEM\_FILE
 * RESIDUAL\_THICKNESS **or** MIN\_RESIDUAL and MAX\_RESIDUAL
 * NEW\_VENT
 * VENT\_EASTING
 * VENT\_NORTHING
 * VENT\_PULSE\_VOLUME **or** MIN\_PULSE\_VOLUME and MAX\_PULSE\_VOLUME
 * VENT\_TOTAL\_VOLUME **or** MIN\_TOTAL\_VOLUME and MAX\_TOTAL\_VOLUME
 * SIMULATIONS and
 * at least one Output file format
 
If Total Flow Volume and/or Residual Flow Thicknesses should be modeled as log-normal distributions, these optional parameters can be added:
 * LOG\_MEAN\_RESIDUAL
 * LOG\_STD\_DEV\_RESIDUAL
 * LOG\_MEAN\_TOTAL\_VOLUME
 * LOG\_STD\_DEV\_TOTAL\_VOLUME
 
Note: If all the parameters used are set in stone and no "MIN/MAX" parameters are used, this will be very boring.

#### Running a probabilistic model of a lava flow field with many different source locations
The INITIALIZE module will use parameters for a probabilistic flow field model, selecting vents from a location probability map if
 1. The NEW\_VENT parameter is not given in the configuration file

To run this kind of flow model, use the following:
 * DEM\_FILE
 * VENT\_SPATIAL\_DENSITY\_FILE
 * MIN\_RESIDUAL and MAX\_RESIDUAL
 * MIN\_PULSE\_VOLUME and MAX\_PULSE\_VOLUME
 * MIN\_TOTAL\_VOLUME and MAX\_TOTAL\_VOLUME
 * SIMULATIONS and
 * at least one Output file format
 
If Total Flow Volume and/or Residual Flow Thicknesses should be modeled as log-normal distributions, these optional parameters can be added:
 * LOG\_MEAN\_RESIDUAL
 * LOG\_STD\_DEV\_RESIDUAL
 * LOG\_MEAN\_TOTAL\_VOLUME
 * LOG\_STD\_DEV\_TOTAL\_VOLUME

### <a name="summarytables"></a> Summary Parameter Table
**Elevation Map Parameters**

| Parameter | Single Flow | Probabilistic Single Flow | Probabilistic Flow Field |
| --- | :---: | :---: | :---: |
| DEM\_FILE | Required    | Required      | Required      |
| ELEVATION\_UNCERT | Not suggested |   Optional    | Optional  |

**Output Parameters**

| Parameter | Single Flow | Probabilistic Single Flow | Probabilistic Flow Field |
| --- | :---: | :---: | :---: |
| ASCII\_THICKNESS\_LIST          | USE         |               |               |
| ASCII\_HIT\_LIST                |             | IF            |               |
| TIFF\_HIT\_MAP                  |             |               | NEEDED        |
| TIFF\_THICKNESS\_MAP            | MUST        |               |               |
| TIFF\_ELEVATION\_MAP            |             | CHOOSE        |               |
| TIFF\_NEW\_ELEV\_MAP            |             |               | AT LEAST 1    |
| STATS\_FILE | Optional | Optional | Optional |

**Determined Flow Parameters**

| Parameter | Single Flow | Probabilistic Single Flow | Probabilistic Flow Field |
| --- | :---: | :---: | :---: |
| RESIDUAL\_THICKNESS             | Required    | Optional      | Do Not Use    |
| NEW\_VENT                       | Required    | Required      | Do Not Use    |
| VENT\_EASTING                   | Required    | Required      | Do Not Use    |
| VENT\_NORTHING                  | Required    | Required      | Do Not Use    |
| VENT\_PULSE\_VOLUME             | Required    | Optional      | Do Not Use    |
| VENT\_TOTAL\_VOLUME             | Required    | Optional      | Do Not Use    |

**Probabilistic Parameters**

| Parameter | Single Flow | Probabilistic Single Flow | Probabilistic Flow Field |
| --- | :---: | :---: | :---: |
| SIMULATIONS                     | Do Not Use | Required (>1) | Required (>1) |
| VENT\_SPATIAL\_DENSITY\_FILE    | Do Not Use | Do Not Use | Required |
| MIN\_RESIDUAL                   | Do Not Use | Optional | Required |
| MAX\_RESIDUAL                   | Do Not Use | Optional | Required |
| LOG\_MEAN\_RESIDUAL             | Do Not Use | Optional | Optional |
| LOG\_STD\_DEV\_RESIDUAL         | Do Not Use | Optional | Optional |
| MIN\_TOTAL\_VOLUME              | Do Not Use | Optional | Required |
| MAX\_TOTAL\_VOLUME              | Do Not Use | Optional | Required |
| LOG\_MEAN\_TOTAL\_VOLUME        | Do Not Use | Optional | Optional |
| LOG\_STD\_DEV\_TOTAL\_VOLUME    | Do Not Use | Optional | Optional |
| MIN\_PULSE\_VOLUME              | Do Not Use | Optional | Required |
| MAX\_PULSE\_VOLUME              | Do Not Use | Optional | Required |

### <a name="initializemodule"></a> How the Initialize Module uses Parameters
IF the SIMULATIONS variable is set to 1 or omitted from the configuration file:  
 * Determined Parameters (NEW\_VENT, VENT\_*, RESIDUAL\_THICKNESS) are required.

IF the SIMULATIONS variable is set to greater than 1 and NEW\_VENT is selected at least once: 
 * The Vent Probability Map will not be used
 * Vent(s) Location variables VENT\_EASTING, VENT\_NORTHING are required
 * Probabilistic parameters will be preferred over Determined parameters (i.e., if probabilistic parameters are set, determined parameters will be reset)
 
IF the SIMULATIONS variable is set to greater than 1 and NEW\_VENT is omitted in the configuration file:
 * A Vent Probability Map is required
 * Minimum/Maximum Flow parameters are required
 * Determined flow parameters will not be used

