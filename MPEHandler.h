#include "Visualiser.h"
#include "TrackHandler.h"

class MPEHandler : public Component,
				   public MPEInstrument::Listener,
				   public Timer{
	public:

		bool started = false;
		
		Visualiser *visualiser;
		TrackHandler *trackHandle;
		MainContentComponent *mainComponent;
        vector<MPENote> notesDown;
        vector<bool> notesDownValid;
        MPEHandler(MainContentComponent *mainComp) {
			mainComponent = mainComp;
			visualiser = new Visualiser(mainComponent);
			trackHandle = new TrackHandler(mainComponent);
            notesDown.assign(16,MPENote());
            notesDownValid.assign(16,false);
		}

		void recievedMidiMiddleProcess(MPENote note, int eventCalledFrom, MidiOutput *midiOutputDevice){
			struct DrawCallback : public CallbackMessage
			{
				int eventCalledFrom;
				MidiOutput *midiOutputDevice;
				DrawCallback(MPEHandler& o, const MPENote& n, int ECF,Visualiser* v, MidiOutput *MOD) : owner(o), note(n), vis(v) {
					eventCalledFrom = ECF;
					midiOutputDevice = MOD;
				}

				void messageCallback() override
				{
					if (eventCalledFrom != 6)
					{
						MidiMessage newMidiMessage = owner.mainComponent->MPEToMidiMessage(note, eventCalledFrom);
						midiOutputDevice->sendMessageNow(newMidiMessage);
					}
					
					owner.visualiser->drawNote(note, eventCalledFrom);
					owner.trackHandle->playedNotes.push_back(note);
					owner.trackHandle->playedNoteTimes.push_back(vis->timeCount);
					owner.trackHandle->playedNoteEvents.push_back(eventCalledFrom);
				}
				MPEHandler& owner;
				Visualiser* vis;
				MPENote note;
			};

			(new DrawCallback(*this, note, eventCalledFrom, visualiser, midiOutputDevice))->post();
		}

		void startNoteDownTimer(MPENote note) {
            notesDown[note.midiChannel] = note;
            notesDownValid[note.midiChannel] = true;
			if (isTimerRunning() == false) {
				startTimer(1);
			}
		}
		void noteAdded(MPENote note) override {
			startNoteDownTimer(note);
			recievedMidiMiddleProcess(note, 1, mainComponent->midiOutputDevice);
		}
		void notePressureChanged(MPENote note) override {
			startNoteDownTimer(note);
			recievedMidiMiddleProcess(note, 2, mainComponent->midiOutputDevice);
		}
		void notePitchbendChanged(MPENote note) override {
			startNoteDownTimer(note);
			recievedMidiMiddleProcess(note, 3, mainComponent->midiOutputDevice);
		}
		void noteTimbreChanged(MPENote note) override {
			startNoteDownTimer(note);
			recievedMidiMiddleProcess(note, 4, mainComponent->midiOutputDevice);
		}
		void noteKeyStateChanged(MPENote note) override {
			//recievedMidiMiddleProcess(note,5);
		}
		void noteReleased(MPENote note) override {
            notesDown[note.midiChannel] = MPENote();
            notesDownValid[note.midiChannel] = false;
            for(int i = 0;i < notesDownValid.size(); i++){
                if(notesDownValid[i] == true){
                    break;
                }else if(i == notesDownValid.size() -1){
                    stopTimer();
                }
            }
			recievedMidiMiddleProcess(note, 5, mainComponent->midiOutputDevice);
		}

		void timerCallback() override
		{
            for(int i = 0;i < notesDown.size();i++){
                if (trackHandle->playedNotes.size() > 1 && notesDownValid[i]==true) {
                    recievedMidiMiddleProcess(notesDown[i], 6, mainComponent->midiOutputDevice);
                }
            }
		}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPEHandler)
};
