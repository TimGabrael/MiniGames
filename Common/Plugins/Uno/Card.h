#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include "Graphics/Camera.h"
#include "Animator.h"
#include "Pointer.h"
#include "Graphics/Helper.h"

static float g_cardScale = 0.3f;
static constexpr float g_cardMinDiff = 0.35f;

enum CARD_ID
{
	CARD_ID_RED_0, CARD_ID_RED_1, CARD_ID_RED_2, CARD_ID_RED_3, CARD_ID_RED_4, CARD_ID_RED_5, CARD_ID_RED_6,
	CARD_ID_RED_7, CARD_ID_RED_8, CARD_ID_RED_9, CARD_ID_RED_PAUSE, CARD_ID_RED_SWAP, CARD_ID_RED_ADD,
	CARD_ID_CHOOSE_COLOR,
	CARD_ID_YELLOW_0, CARD_ID_YELLOW_1, CARD_ID_YELLOW_2, CARD_ID_YELLOW_3, CARD_ID_YELLOW_4, CARD_ID_YELLOW_5, CARD_ID_YELLOW_6,
	CARD_ID_YELLOW_7, CARD_ID_YELLOW_8, CARD_ID_YELLOW_9, CARD_ID_YELLOW_PAUSE, CARD_ID_YELLOW_SWAP, CARD_ID_YELLOW_ADD,
	CARD_ID_ADD_4,
	CARD_ID_GREEN_0, CARD_ID_GREEN_1, CARD_ID_GREEN_2, CARD_ID_GREEN_3, CARD_ID_GREEN_4, CARD_ID_GREEN_5, CARD_ID_GREEN_6,
	CARD_ID_GREEN_7, CARD_ID_GREEN_8, CARD_ID_GREEN_9, CARD_ID_GREEN_PAUSE, CARD_ID_GREEN_SWAP, CARD_ID_GREEN_ADD,
	
	CARD_ID_BLANK,

	CARD_ID_BLUE_0, CARD_ID_BLUE_1, CARD_ID_BLUE_2, CARD_ID_BLUE_3, CARD_ID_BLUE_4, CARD_ID_BLUE_5, CARD_ID_BLUE_6,
	CARD_ID_BLUE_7, CARD_ID_BLUE_8, CARD_ID_BLUE_9, CARD_ID_BLUE_PAUSE, CARD_ID_BLUE_SWAP, CARD_ID_BLUE_ADD,

	CARD_ID_BLANK2,


	NUM_AVAILABLE_CARDS,
};

enum CARD_EFFECT
{
	CARD_EFFECT_BLUR,
	NUM_EFFECTS,
};

enum CARD_ANIMATIONS
{
	ANIM_PLAY_CARD,		// throw a card on the board
	ANIM_FETCH_CARD,	// get a card from the deck
	ANIM_UNPLAYABLE,	// shake card if it is not playable
};

struct CardSceneObject	// doesn't have any data in it, uses the global card data to draw batches
{
	BaseSceneObject base;
};



void InitializeCardPipeline(void* assetManager);

void RendererAddCard(CARD_ID back, CARD_ID front, const glm::vec3& pos, const glm::quat& rot, float sx, float sy);
void RendererAddEffect(CARD_EFFECT effect, const glm::vec3& pos, const glm::quat& rot, float sx, float sy, uint32_t col);
void ClearCards();


void AddCardTypeToScene(PScene scene);
CardSceneObject* CreateCardBatchSceneObject(PScene scene);


void DrawCards(const glm::mat4& proj, const glm::mat4& view, const glm::vec3& camPos);

bool HitTest(const glm::mat4& model, const glm::vec3& camPos, const glm::vec3& mouseRay);


// The Deck consists of the official UNO deck specification, is stored IN ORDER, blank cards are not included
// https://www.unorules.org/how-many-cards-in-uno/
std::vector<CARD_ID> GenerateDeck();


bool CardSort(CARD_ID low, CARD_ID high);

// color refrence card is used for black cards, (user choice)
bool CardIsPlayable(CARD_ID topCard, CARD_ID playing, CARD_ID colorRefrenceCard);

uint32_t GetCardColorFromID(CARD_ID id);




struct ColorPicker
{
	static constexpr float o = 0.01f;
	static constexpr float ho = 0.0f;
	static constexpr float r = 0.4f;
	static constexpr float hr = 0.4f + o * 2.8f;

	int hoveredColor = 0;
	int pressedColor= false;	// partially selected a color
	float hoverTimer = 0.0f;
	bool isCurrentlyHovered = false;
	CARD_ID GetSelected(int mx, int my, int screenX, int screenY, bool pressed, bool released);
	void Draw(float aspectRatio, float dt);
};





struct CardsInAnimation;
struct CardInfo
{
	CardInfo(CARD_ID back, CARD_ID front, const glm::vec3& pos, const glm::quat& rot, float hov, bool isVisible) : back(back), front(front), position(pos), rotation(rot), hoverCounter(hov), visible(isVisible) {};
	CARD_ID back; CARD_ID front;
	glm::vec3 position;
	glm::quat rotation;
	float hoverCounter = 0.0f;
	int transitionID = -1;			// used to match a card with a animation
	bool visible = false;			// temporary card is not shown! used for animations that end in the hand
};

struct CardDeck
{
	static constexpr float dx = -0.6f;
	static constexpr float dy = 0.003f;
	static constexpr int numCards = 40;
	static constexpr float topY = dy * numCards;
	std::vector<CARD_ID> deck;
	void Draw();
	CARD_ID PullCard();
};
struct CardStack
{
	static constexpr float dy = 0.003f;
	std::vector<CardInfo> cards;
	CARD_ID blackColorID;
	
	float topAnim = 0.0f;
	bool countDown = false;

	void Draw();
	void AddToStack(CARD_ID card, const glm::vec3& pos, const glm::quat& rot);
	CARD_ID GetTop(CARD_ID& blackColorRef) const;
};
struct CardHand
{
	CardHand(int id) { 
		highlightedCardIdx = -1; maxWidth = 0.0f; wideCardsStart = -1.0f; currentAssignedAnimID = 1; mouseSelectedCard = -1; mouseAttached = false;
		needRegen = false; handID = id;
	}
	std::vector<CardInfo> cards;
	int highlightedCardIdx = -1;
	// used when the number of cards on hand are overflowing the visible capacity
	float maxWidth = 0.0f;
	float wideCardsStart = -1.0f;

	int currentAssignedAnimID = 1;
	int mouseSelectedCard = -1;

	int handID = 0;
	float rotation = 0.0f;
	
	CARD_ID choosenCardColor = CARD_ID_RED_2;
	bool choosingCardColor = false;

	bool mouseAttached = false;

	bool needRegen = false;
	void Add(CARD_ID id);
	// returns the index that the card got placed in
	int AddTemp(const Camera& cam, CARD_ID id);
	void PlayCard(const CardStack& stack, CardsInAnimation& anim, int cardIdx);
	void FetchCard(const Camera& cam, const CardStack& stack, CardDeck& deck, CardsInAnimation& anim);
	void Update(CardStack& stack, CardsInAnimation& anim, ColorPicker& picker, const Camera& cam, const glm::vec3& mouseRay, const Pointer& p, bool allowInput);
	void GenTransformations(const Camera& cam);
	void Draw(const Camera& cam);
};


struct AnimatedCard
{
	AnimatedCard(const CardStack& stack, CARD_ID front, const glm::vec3& pos, const glm::quat& rot, float dur, CARD_ANIMATIONS a, int cardID, int handID);
	void AddStackAnimation(const CardStack& stack);
	void AddDeckAnimation();

	CARD_ID back; CARD_ID front;
	Animation<glm::vec3> posAnim;
	Animation<glm::quat> rotAnim;
	CARD_ANIMATIONS type;
	int handID = -1;
	int cardID = 0;
	bool done = false;
};
struct CardsInAnimation
{
	std::vector<AnimatedCard> list;
	bool inputsAllowed;
	void AddAnim(const CardStack& stack, const CardInfo& info, int handID, CARD_ANIMATIONS id);
	void Update(std::vector<CardHand>& hands, CardStack& stack, float dt);
	void OnFinish(std::vector<CardHand>& hands, CardStack& stack, const glm::vec3& position, const glm::quat& rotation, CARD_ID id, CARD_ANIMATIONS type, int handID, int transitionID);
};





