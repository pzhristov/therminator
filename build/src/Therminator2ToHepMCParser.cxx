#include "Therminator2ToHepMCParser.h"

using namespace std;
using namespace HepMC;

Therminator2ToHepMCParser::Therminator2ToHepMCParser():_HepMC_writer(0)
{
    
}

Therminator2ToHepMCParser::Therminator2ToHepMCParser(string inputFileName, string outputFileName):_HepMC_writer(0)
{
    this->inputFileName = inputFileName;
    this->outputFileName = outputFileName;
}

void Therminator2ToHepMCParser::Run()
{
    const double fm_to_mm = 1.e-12;

    if(!initParser())
        return;

    string lineBuffer;
    string temp;

    int events = 0;
    
    cout<<"Reading events..."<<endl;

    bool isFifo = true;

    while(true)     //Event loop
    {
        if(eidToVertex)
            delete eidToVertex;
        eidToVertex = new unordered_map<int, GenVertex*>();

        if(isFifo || events==0)
        {
            inputStream.open(inputFileName.c_str());
            if(!inputStream.good())
            {
                cerr<<"Error, could not open file "<<inputFileName<<endl;
                return;
            }
        }

        getline(inputStream, lineBuffer);      //Skip first line of event header
        if((events==0 || !isFifo) && !inputStream.good())  //If it's a normal file, check if it's the end
        {                                   //If it works in fifo mode, the parser will be killed
            inputStream.close();            //by AliRoot
            break;
        }

        if(events==0 && lineBuffer[1] == ' ')  //Check for extra lines
        {                                      //When Therminator2 opens the file by itself, it adds
            isFifo=false;                      //two extra lines, so at the beginning of the file there are
            getline(inputStream, lineBuffer);  //extra lines that we must skip                  
            getline(inputStream, lineBuffer);                    
                                            
        }                                   
        
        inputStream>>temp>>_particlesInEvent;   //Get number of particles
        getline(inputStream, lineBuffer);       //Skip the rest of the line
        
        cout<<"Event "<<events+1<<": Number of particles in event: "<<_particlesInEvent<<endl;


        GenEvent parsed_event(Units::GEV, Units::MM);
        
        parsed_event.weights().push_back(1.);

        // Add Heavy Ion information. This may not be required
        HeavyIon heavyion(
            -1,                                     // Number of hard scatterings
            -1,                                     // Number of projectile participants
            -1,                                     // Number of target participants
            -1,                                     // Number of NN (nucleon-nucleon) collisions
            -1,                                     // Number of N-Nwounded collisions
            -1,                                     // Number of Nwounded-N collisons
            -1,                                     // Number of Nwounded-Nwounded collisions
            -1,                                     // Number of spectator neutrons
            -1,                                     // Number of spectator protons
            -1,                                     // Impact Parameter(fm) of collision
            0.,                                     // Azimuthal angle of event plane
            -1.,                                    // eccentricity of participating nucleons
                                                            //in the transverse plane 
                                                            //(as in phobos nucl-ex/0510031) 
            -1.);                                   // nucleon-nucleon inelastic 
                                                            //(including diffractive) cross-section
        parsed_event.set_heavy_ion(heavyion);
        

        // adding two `artificial' beam particles. the hepmc reader in aliroot expects two beam particles
        // (which in hepmc have a special status) ,so we need to create them. they will not 
        // end up in the analysis, as they have only longitudinal momentum. 
        GenVertex* v = new GenVertex();
        parsed_event.add_vertex(v);

        FourVector beam1(0,0,.14e4, 1686.2977);
        GenParticle* bp1 = new GenParticle(beam1, 2212, 2);
        bp1->set_generated_mass(940);
        v->add_particle_in(bp1);

        FourVector beam2(0,0,.14e4, 1686.2977);
        GenParticle* bp2 = new GenParticle(beam2, 2212, 2);
        bp2->set_generated_mass(940);
        v->add_particle_in(bp2);

        parsed_event.set_beam_particles(bp1,bp2);
        

        //Particle Loop
        for(int i=0; i<_particlesInEvent; i++)
        {
            fillParticle();

            FourVector fourmom(
                    _particle.px,
                    _particle.py, 
                    _particle.pz, 
                    _particle.energy);
            GenParticle* particle = new GenParticle(fourmom, _particle.pid, _particle.decayed+1);
            particle->set_generated_mass(_particle.mass);

            GenVertex* out_vertex;

            // Determine the production vertex. If it's not the primordial particle, look it up
            if(_particle.fathereid != -1)
                out_vertex = eidToVertex->at(_particle.fathereid);
            else
            {
                // If it's a primordial particle, create a pseudo particle for connection
                // and its production vertex. 
                FourVector hyperVector(0, 0, 0, 0);
                GenParticle* hypersurface = new GenParticle(hyperVector, 0, 0);
                v->add_particle_out(hypersurface);

                out_vertex = new GenVertex();
                out_vertex->add_particle_in(hypersurface);
                parsed_event.add_vertex(out_vertex);
                FourVector spaceTimePos(
                            _particle.x*fm_to_mm,
                            _particle.y*fm_to_mm, 
                            _particle.z*fm_to_mm, 
                            _particle.t*fm_to_mm);
                out_vertex->set_position(spaceTimePos);
            }
            
            // If not already done, set the creation position
            if(out_vertex->position() == FourVector(0,0,0,0))
            {
                FourVector spaceTimePos(
                            _particle.x*fm_to_mm,
                            _particle.y*fm_to_mm, 
                            _particle.z*fm_to_mm, 
                            _particle.t*fm_to_mm);

                out_vertex->set_position(spaceTimePos);
            }

            out_vertex->add_particle_out(particle);


            // If particle decayed, create a decay vertex and store it for future use
            if(_particle.decayed)
            {   
                GenVertex* vert = new GenVertex();
                vert->add_particle_in(particle);
                parsed_event.add_vertex(vert);
                eidToVertex->emplace(_particle.eid, vert);  // Child particles will use this vertex 
                                                            // for lookup
            }

        }
 
        _HepMC_writer->write_event(&parsed_event);
        events++;
        getline(inputStream, lineBuffer);  //Skip last line of particles

        if(isFifo)
            inputStream.close();

    }

    cout<<"End. Events parsed: "<<events<<endl;
        
}

bool Therminator2ToHepMCParser::initParser()
{

    _HepMC_writer = new IO_GenEvent(outputFileName);
    if(!_HepMC_writer)
    {
        cerr<<"Error, could not initalize hepmc writer"<<endl;
        return false;
    }

    return true;
}

void Therminator2ToHepMCParser::fillParticle()
{
   inputStream>>_particle.eid>>_particle.fathereid>>_particle.pid>>_particle.fatherpid;
   inputStream>>_particle.rootpid>>_particle.decayed>>_particle.mass>>_particle.energy;
   inputStream>>_particle.px>>_particle.py>>_particle.pz>>_particle.t;
   inputStream>>_particle.x>>_particle.y>>_particle.z;
}
