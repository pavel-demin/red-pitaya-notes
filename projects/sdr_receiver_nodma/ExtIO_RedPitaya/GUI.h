#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace ExtIO_RedPitaya {

	/// <summary>
	/// Summary for GUI
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class GUI : public System::Windows::Forms::Form
	{
	public:
		GUI(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			rateCallback = 0;
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~GUI()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Label^  addrLabel;
	public: System::Windows::Forms::TextBox^  addrValue;
	private: System::Windows::Forms::Label^  rateLabel;
	public: System::Windows::Forms::ComboBox^  rateValue;

	public: void (*rateCallback)(UInt32);

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->addrLabel = (gcnew System::Windows::Forms::Label());
			this->addrValue = (gcnew System::Windows::Forms::TextBox());
			this->rateLabel = (gcnew System::Windows::Forms::Label());
			this->rateValue = (gcnew System::Windows::Forms::ComboBox());
			this->SuspendLayout();
			// 
			// addrLabel
			// 
			this->addrLabel->AutoSize = true;
			this->addrLabel->Location = System::Drawing::Point(12, 15);
			this->addrLabel->Name = L"addrLabel";
			this->addrLabel->Size = System::Drawing::Size(57, 13);
			this->addrLabel->TabIndex = 0;
			this->addrLabel->Text = L"IP address";
			// 
			// addrValue
			// 
			this->addrValue->Location = System::Drawing::Point(81, 12);
			this->addrValue->Name = L"addrValue";
			this->addrValue->Size = System::Drawing::Size(101, 20);
			this->addrValue->TabIndex = 1;
			// 
			// rateLabel
			// 
			this->rateLabel->AutoSize = true;
			this->rateLabel->Location = System::Drawing::Point(12, 41);
			this->rateLabel->Name = L"rateLabel";
			this->rateLabel->Size = System::Drawing::Size(63, 13);
			this->rateLabel->TabIndex = 3;
			this->rateLabel->Text = L"Sample rate";
			// 
			// rateValue
			// 
			this->rateValue->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->rateValue->FormattingEnabled = true;
			this->rateValue->Items->AddRange(gcnew cli::array< System::Object^  >(4) {L"50 kSPS", L"100 kSPS", L"250 kSPS", L"500 kSPS"});
			this->rateValue->Location = System::Drawing::Point(81, 38);
			this->rateValue->Name = L"rateValue";
			this->rateValue->Size = System::Drawing::Size(101, 21);
			this->rateValue->TabIndex = 2;
			this->rateValue->SelectedIndexChanged += gcnew System::EventHandler(this, &GUI::bwValue_SelectedIndexChanged);
			// 
			// GUI
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(194, 70);
			this->Controls->Add(this->rateLabel);
			this->Controls->Add(this->rateValue);
			this->Controls->Add(this->addrValue);
			this->Controls->Add(this->addrLabel);
			this->Name = L"GUI";
			this->Text = L"Settings";
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void bwValue_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
				 if(rateCallback) (*rateCallback)(rateValue->SelectedIndex);
			 }
	};
}
