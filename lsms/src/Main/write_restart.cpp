/* -*- c-file-style: "bsd"; c-basic-offset: 2; indent-tabs-mode: nil -*- */

#include <stdio.h>
#include "mixing.hpp"
#include "LSMSMode.hpp"

int writeRestart(const char *restartName, LSMSSystemParameters &lsms, CrystalParameters &crystal, MixingParameters &mix,
                 PotentialShifter &potentialShifter, AlloyMixingDesc& alloyDesc)
{
  // write input file for restart
  if(lsms.pot_out_type < 0) return 1; // if we didn't write output files we probably don't want to write a restart either

  FILE *of = fopen(restartName,"w");

  fprintf(of, "-- Restart input file generated by LSMS\n");
  
  fprintf(of, "systemid=\"%s\"\n",lsms.systemid);
  fprintf(of,"system_title=\"%s\"\n",lsms.title);
  // luaGetStrN(L,"potential_file_in",lsms.potential_file_in,128);
  // luaGetStrN(L,"potential_file_out",lsms.potential_file_out,128);

  fprintf(of,"pot_in_type=%d\n",lsms.pot_out_type);
  fprintf(of,"pot_out_type=%d\n",lsms.pot_out_type);

  fprintf(of,"lsmsMode=\"%s\"\n",lsmsModeToString(lsms.lsmsMode));
  fprintf(of,"istop=\"main\"\n\n");

  fprintf(of,"nscf=%d\n", lsms.nscf);
  fprintf(of,"writeSteps=%d\n\n", lsms.writeSteps);
  fprintf(of,"rmsTolerance=%lg\n\n",lsms.rmsTolerance);

  fprintf(of,"print_node=%d\n",lsms.global.print_node);
  fprintf(of,"default_iprint=%d\n",lsms.global.default_iprint);
  fprintf(of,"iprint=%d\n\n",lsms.global.iprint);

  fprintf(of,"iprpts=%d\n",lsms.global.iprpts);
  fprintf(of,"ipcore=%d\n\n",lsms.global.ipcore);
  
  if(lsms.infoEvecFileIn[0]!=0)
    fprintf(of,"infoEvecFileIn=\"%s\"\n",lsms.infoEvecFileIn);
  fprintf(of,"infoEvecFileOut=\"%s\"\n\n",lsms.infoEvecFileOut);

  if(lsms.localAtomDataFile[0]!=0)
    fprintf(of,"localAtomDataFile=\"%s\"\n\n",lsms.localAtomDataFile);
  
  fprintf(of,"gpu_threads=%d\n",lsms.global.GPUThreads);

  fprintf(of,"num_atoms=%d\n", lsms.num_atoms);

  if(lsms.relativity == none)
  {
    fprintf(of,"relativity=\"none\"\n");
  } else if(lsms.relativity == scalar) {
    fprintf(of, "relativity=\"scalar\"\n");
  } else if(lsms.relativity == full) {
    fprintf(of, "relativity=\"full\"\n");
  }

  if(lsms.nrelc == 0)
  {
    fprintf(of,"core_relativity=\"full\"\n");
  } else {
    fprintf(of,"core_relativity=\"none\"\n");
  }
  fprintf(of,"mtasa=%d\n",lsms.mtasa);
  fprintf(of,"fixRMT=%d\n",lsms.fixRMT);
  fprintf(of,"nspin=%d\n",lsms.nspin);

  fprintf(of,"xcFunctional={%d",lsms.xcFunctional[0]);
  
  for(int i=1; i<numFunctionalIndices; i++)
  {
    if(lsms.xcFunctional[i]>=0)
      fprintf(of,",%d",lsms.xcFunctional[i]);
  }
  fprintf(of, "}\n\n");

  fprintf(of,"energyContour={}\n");
  fprintf(of,"energyContour.grid=%d\n",lsms.energyContour.grid);
  fprintf(of,"energyContour.npts=%d\n",lsms.energyContour.npts);
  fprintf(of,"energyContour.ebot=%lf\n",lsms.energyContour.ebot);
  fprintf(of,"energyContour.etop=%lf\n",lsms.energyContour.etop);
  fprintf(of,"energyContour.eibot=%lf\n",lsms.energyContour.eibot);
  fprintf(of,"energyContour.eitop=%lf\n",lsms.energyContour.eitop);
  fprintf(of,"energyContour.maxGroupSize=%d\n\n",lsms.energyContour.maxGroupSize);

  fprintf(of,"adjustContourBottom=%lf\n\n",lsms.adjustContourBottom);

  fprintf(of,"temperature=%lf\n\n",lsms.temperature);


  fprintf(of,"mixing={ ");
  bool printComma=false;
  int  numberOfMixQuantities=0;
  for(int idx=0; idx<mix.numQuantities; idx++)
  {
    if(mix.quantity[idx])
    {
      if(printComma) fprintf(of,", ");
      printComma=true;
      numberOfMixQuantities++;
      if(idx == MixingParameters::charge)
      {
        fprintf(of, "{quantity = \"charge\", ");
      } else if(idx == MixingParameters::potential) {
        fprintf(of, "{quantity = \"potential\", ");
      } else if(idx == MixingParameters::moment_magnitude) {
         fprintf(of, "{quantity = \"moment_magnitude\", ");
      } else if(idx == MixingParameters::moment_direction) {
        fprintf(of, "{quantity = \"moment_direction\", ");
      } else if(idx == MixingParameters::no_mixing) {
        fprintf(of, "{quantity = \"no_mixing\", ");
      }
      if(mix.algorithm[idx] == MixingParameters::simple)
      {
        fprintf(of, "algorithm = \"simple\", ");
      } else if(mix.algorithm[idx] == MixingParameters::broyden) {
        fprintf(of, "algorithm = \"broyden\", ");
      } else {
        fprintf(of, "algorithm = \"noAlgorithm\", ");
      }
      fprintf(of, "mixing_parameter = %lf}",mix.mixingParameter[idx]);
    }
  }
  fprintf(of, "}\n");
  
  fprintf(of,"numberOfMixQuantities=%d\n\n", numberOfMixQuantities);
  
  fprintf(of, "bravais={}\n");
  for(int i=0; i<3; i++)
  {
    fprintf(of,"bravais[%d]={%21.15lf, %21.15lf, %21.15lf}\n",i+1,
            crystal.bravais(0,i),crystal.bravais(1,i),crystal.bravais(2,i));
  }

  fprintf(of, "\nsite={}\n");
  fprintf(of, "for i =1,num_atoms do site[i]={} end\n\n");
  for(int i=0; i<crystal.num_atoms; i++)
  {
    int t; // the type to be assigned

    fprintf(of,"site[%d].pos={%21.15lf, %21.15lf, %21.15lf}\n",
            i+1,crystal.position(0,i),crystal.position(1,i),crystal.position(2,i));
    fprintf(of,"site[%d].evec={%21.15lf, %21.15lf, %21.15lf}\n",
            i+1,crystal.evecs(0,i),crystal.evecs(1,i),crystal.evecs(2,i));
    // if(!luaGetIntegerFieldFromStack(L,"type",&t) || (t-1)==i)
    fprintf(of,"site[%d].atom=\"%s\"\n",i+1,crystal.types[crystal.type[i]].name);
    fprintf(of,"site[%d].lmax=%d\n",i+1,crystal.types[crystal.type[i]].lmax);
    fprintf(of,"site[%d].Z=%d\n",i+1,crystal.types[crystal.type[i]].Z);
    fprintf(of,"site[%d].Zc=%d\n",i+1,crystal.types[crystal.type[i]].Zc);
    fprintf(of,"site[%d].Zs=%d\n",i+1,crystal.types[crystal.type[i]].Zs);
    fprintf(of,"site[%d].Zv=%d\n",i+1,crystal.types[crystal.type[i]].Zv);
    // luaGetIntegerFieldFromStack(L,"pot_in_idx",&crystal.types[crystal.type[i]].pot_in_idx);
    if(crystal.types[crystal.type[i]].forceZeroMoment!=0)
      fprintf(of,"site[%d].forceZeroMoment=%d\n",i+1,crystal.types[crystal.type[i]].forceZeroMoment);
    fprintf(of,"site[%d].rLIZ=%lf\n",i+1,crystal.types[crystal.type[i]].rLIZ);
    fprintf(of,"site[%d].rsteps={%lf,%lf,%lf,%lf}\n",i+1,
            crystal.types[crystal.type[i]].rsteps[0],crystal.types[crystal.type[i]].rsteps[1],
            crystal.types[crystal.type[i]].rsteps[2],crystal.types[crystal.type[i]].rsteps[3]);
    fprintf(of,"site[%d].rad=%lf\n",i+1,crystal.types[crystal.type[i]].rad);
    
    // if(!luaGetIntegerFieldFromStack(L,"type",&t) || (t-1)==i)
 
    // luaGetIntegerFieldFromStack(L,"alloy_class",&crystal.types[crystal.num_types].alloy_class);
    // crystal.types[crystal.num_types].alloy_class--; // <-- zero-based indexing
    

  }

  fclose(of);
  return 0;
}
